// -*- coding: utf-8 -*-
// Copyright (C) 2018 Laboratoire de Recherche et Développement
// de l'Epita (LRDE).
//
// This file is part of Spot, a model checking library.
//
// Spot is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// Spot is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
// or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
// License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "config.h"
#include "gfguarantee.hh"
#include <spot/twa/twagraph.hh>
#include <spot/twaalgos/sccinfo.hh>
#include <spot/twaalgos/isweakscc.hh>
#include <spot/twaalgos/strength.hh>
#include <spot/twaalgos/ltl2tgba_fm.hh>
#include <spot/twaalgos/minimize.hh>
#include <spot/twaalgos/dualize.hh>
#include <spot/twaalgos/isdet.hh>

namespace spot
{
  namespace
  {
    // F(φ₁)&F(φ₂)&F(φ₃) ≡ F(φ₁ & F(φ₂ & F(φ₃))
    // because we assume this is all under G.
    static formula
    nest_f(formula input)
    {
      assert(input.is(op::And));
      formula res = formula::tt();
      unsigned n = input.size();
      do
        {
          --n;
          assert(input[n].is(op::F));
          res = formula::F(formula::And({input[n][0], res}));
        }
      while (n);
      return res;
    }


    static twa_graph_ptr
    do_g_f_terminal_inplace(scc_info& si, bool state_based)
    {
      twa_graph_ptr aut = std::const_pointer_cast<twa_graph>(si.get_aut());

      if (!is_terminal_automaton(aut, &si, true))
        throw std::runtime_error("g_f_terminal() expects a terminal automaton");

      unsigned ns = si.scc_count();
      std::vector<bool> term(ns, false);
      for (unsigned n = 0; n < ns; ++n)
        if (is_terminal_scc(si, n))
          term[n] = true;

      aut->prop_keep({ false, false, true, false, true, true });
      aut->prop_state_acc(state_based);
      aut->prop_inherently_weak(false);
      aut->set_buchi();

      unsigned init = aut->get_init_state_number();

      if (!state_based)
        {
          bool is_det = is_deterministic(aut);
          bool initial_state_reachable = false;
          for (auto& e: aut->edges())
            if (e.dst == init)
              {
                initial_state_reachable = true;
                break;
              }
          unsigned nedges = aut->num_edges();
          unsigned new_init = -1U;
          // The loop might add new edges, but we just want to iterate
          // on the initial ones.
          for (unsigned edge = 1; edge <= nedges; ++edge)
            {
              auto& e = aut->edge_storage(edge);
              if (term[si.scc_of(e.dst)])
                {
                  if (initial_state_reachable
                      // If the source state is the initial state, we
                      // should not try to combine it with itself...
                      || e.src == init)
                    {
                      e.dst = init;
                      e.acc = {0};
                      new_init = init;
                      continue;
                    }
                  // If initial state cannot be reached from another
                  // state of the automaton, we can get rid of it by
                  // combining the edge reaching the terminal state
                  // with the edges leaving the initial state.
                  bdd econd = e.cond;
                  bool first = true;
                  // Combine this edge with any compatible edge from
                  // the initial state.
                  for (auto& ei: aut->out(init))
                    {
                      bdd cond = ei.cond & econd;
                      if (cond != bddfalse)
                        {
                          if (first)
                            {
                              e.dst = ei.dst;
                              e.acc = {0};
                              e.cond = cond;
                              first = false;
                              if (new_init == -1U)
                                new_init = e.src;
                            }
                          else
                            {
                              aut->new_edge(e.src, ei.dst, cond, {0});
                            }

                        }
                    }
                }
              else
                {
                  e.acc = {};
                }
            }
          // In a deterministic and suspendable automaton, all states
          // recognize the same language, so we can freely move the
          // initial state.  We decide to use the source of any
          // accepting transition, in the hope that it will make the
          // original initial state unreachable.
          if (is_det && new_init != -1U)
            aut->set_init_state(new_init);
        }
      else
        {
          // Replace all terminal state by a single accepting state.
          unsigned accstate = aut->new_state();
          for (auto& e: aut->edges())
            {
              if (term[si.scc_of(e.dst)])
                e.dst = accstate;
              e.acc = {};
            }
          // This accepting state has the same output as the initial
          // state.
          for (auto& e: aut->out(init))
            aut->new_edge(accstate, e.dst, e.cond, {0});
          // This is not mandatory, but starting on the accepting
          // state helps getting shorter accepting words and may
          // reader the original initial state unreachable, saving one
          // state.
          aut->set_init_state(accstate);
        }

      aut->purge_unreachable_states();
      return aut;
    }
  }

  twa_graph_ptr
  g_f_terminal_inplace(twa_graph_ptr aut, bool state_based)
  {
    scc_info si(aut);
    return do_g_f_terminal_inplace(si, state_based);
  }

  twa_graph_ptr
  gf_guarantee_to_ba_maybe(formula gf, const bdd_dict_ptr& dict,
                           bool deterministic, bool state_based)
  {
    if (!gf.is(op::G))
      return nullptr;
    formula f = gf[0];
    if (!f.is(op::F))
      {
        // F(...)&F(...)&... is also OK.
        if (!f.is(op::And))
          return nullptr;
        for (auto c: f)
          if (!c.is(op::F))
            return nullptr;

        f = nest_f(f);
      }
    twa_graph_ptr aut = ltl_to_tgba_fm(f, dict, true);
    twa_graph_ptr reduced = minimize_obligation(aut, f, nullptr,
                                                !deterministic);
    scc_info si(reduced);
    if (!is_terminal_automaton(reduced, &si, true))
      return nullptr;
    do_g_f_terminal_inplace(si, state_based);
    return reduced;
  }

  twa_graph_ptr
  gf_guarantee_to_ba(formula gf, const bdd_dict_ptr& dict,
                     bool deterministic, bool state_based)
  {
    twa_graph_ptr res = gf_guarantee_to_ba_maybe(gf, dict,
                                                 deterministic, state_based);
    if (!res)
      throw std::runtime_error
        ("gf_guarantee_to_ba(): expects a formula of the form GF(guarantee)");
    return res;
  }

  twa_graph_ptr
  fg_safety_to_dca_maybe(formula fg, const bdd_dict_ptr& dict,
                         bool state_based)
  {
    if (!fg.is(op::F))
      return nullptr;
    formula g = fg[0];
    if (!g.is(op::G))
      {
        // G(...)|G(...)|... is also OK.
        if (!g.is(op::Or))
          return nullptr;
        for (auto c: g)
          if (!c.is(op::G))
            return nullptr;
      }

    formula gf = negative_normal_form(fg, true);
    twa_graph_ptr res =
      gf_guarantee_to_ba_maybe(gf, dict, true, state_based);
    if (!res)
      return nullptr;
    return dualize(res);
  }

  twa_graph_ptr
  fg_safety_to_dca(formula gf, const bdd_dict_ptr& dict,
                   bool state_based)
  {
    twa_graph_ptr res = fg_safety_to_dca_maybe(gf, dict, state_based);
    if (!res)
      throw std::runtime_error
        ("fg_safety_to_dca(): expects a formula of the form FG(safety)");
    return res;
  }
}