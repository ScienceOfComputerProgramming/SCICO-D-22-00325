// -*- coding: utf-8 -*-
// Copyright (C) 2022 Laboratoire de Recherche
// de l'Epita (LRE).
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

#pragma once

#include <bddx.h>
#include <spot/graph/graph.hh>
#include <spot/tl/formula.hh>
#include <spot/tl/relabel.hh>
#include <spot/twa/bdddict.hh>
#include <spot/twa/formula2bdd.hh>
#include <spot/twa/twagraph.hh>
#include <spot/misc/bddlt.hh>

namespace spot
{
  struct SPOT_API bdd_partition
  {
    struct S
    {
      bdd new_label = bddfalse;
    };
    struct T
    {
    };
    using implication_graph = digraph<S, T>;

    // The original conditions and aps to be partitioned
    const std::vector<bdd> all_cond_;
    const std::vector<formula> all_orig_ap_;
    // Graph with the invariant that
    // children imply parents
    // Leaves from the partition
    // original conditions are "root" nodes
    std::unique_ptr<implication_graph> ig;
    // todo: technically there are at most two successors, so a graph
    // is "too" generic
    // All conditions currently part of the partition
    // unsigned corresponds to the associated node
    std::vector<std::pair<bdd, unsigned>> treated;
    std::unordered_map<bdd, unsigned, bdd_hash> all_inter_;
    std::vector<formula> new_aps;
    bool relabel_succ = false;

    bdd_partition()
      : all_cond_()
      , all_orig_ap_()
    {
    }
    bdd_partition(const std::vector<bdd>& all_cond,
                  const std::vector<formula>& all_orig_ap)
      : all_cond_(all_cond)
      , all_orig_ap_(all_orig_ap)
      , ig{std::make_unique<implication_graph>(2*all_cond.size(),
                                              2*all_cond.size())}
    {
      // Create the roots of all old conditions
      // Each condition is associated to the state with
      // the same index
      const unsigned Norig = all_cond.size();
      ig->new_states(Norig);
    }

    // Facilitate conversion
    // This can only be called when letters have already
    // been computed
    relabeling_map
    to_relabeling_map(twa_graph& for_me) const;

    relabeling_map
    to_relabeling_map(const twa_graph_ptr& for_me) const;

    // Dump as hoa to stream
    // Old conditions are shown as state name;
    // New conditions are shown as self-loop
    // Can only be called when leters have already been
    // computed
    void dump(std::ostream& os) const;

    // Verify if condition is valid
    bool verify(bool verbose);

  }; // bdd_partition


  SPOT_API bdd_partition
  try_partition_me(const std::vector<bdd>& all_cond,
                   const std::vector<formula>& ap,
                   unsigned max_letter);
}