// -*- coding: utf-8 -*-
// Copyright (C) 2017-2020 Laboratoire de Recherche et Développement
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

#pragma once

#include <algorithm>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <vector>

#include <bddx.h>
#include <spot/twa/twagraph.hh>
#include <spot/twaalgos/parity.hh>

namespace spot
{

  /// \brief Transform an automaton into a parity game by propagating
  /// players
  ///
  /// This propagate state players, assuming the initial state belong
  /// to \a first_player, and alternating players on each transitions.
  /// If an odd cycle is detected, a runtime_exception is raised.
  /// \param clean_up : If true, the function will try to fix self-loops
  ///                   (split them) and redirect edges to the other
  ///                   state of a sink to prevent odd-cycles
  ///
  ///
  /// If \a complete0 is set, ensure that states of player 0 are
  /// complete.
  SPOT_API
  void alternate_players(spot::twa_graph_ptr& arena,
                         bool first_player = false,
                         bool complete0 = true,
                         bool clean_up = false);


  // false -> env, true -> player
  typedef std::vector<bool> region_t;
  // state idx -> global edge number
  typedef std::vector<unsigned> strategy_t;


  /// \brief solve a parity-game
  ///
  /// The arena is a deterministic max odd parity automaton with a
  /// "state-player" property.
  ///
  /// Player 1 tries to satisfy the acceptance condition, while player
  /// 0 tries to prevent that.
  ///
  /// This computes the winning strategy and winning region using
  /// Zielonka's recursive algorithm.  \cite zielonka.98.tcs
  ///
  /// Also includes some inspiration from Oink.
  /// \cite vandijk.18.tacas
  ///
  /// Returns the player winning in the initial state, and sets
  /// the state-winner and strategy named properties.
  SPOT_API
  bool solve_parity_game(const twa_graph_ptr& arena);

  /// \brief Solve a safety game.
  ///
  /// The arena should be represented by an automaton with true
  /// acceptance.
  ///
  /// Player 1 tries to satisfy the acceptance condition, while player
  /// 0 tries to prevent that.   The only way for player 0 to win is
  /// to find a way to move the play toward a state without successor.
  /// If there no state without successors, then the game is necessarily
  /// winning for player 1.
  ///
  /// Returns the player winning in the initial state, and sets
  /// the state-winner and strategy named properties.
  SPOT_API
  bool solve_safety_game(twa_graph_ptr game);

  /// \brief Print a max odd parity game using PG-solver syntax
  SPOT_API
  void pg_print(std::ostream& os, const const_twa_graph_ptr& arena);


  /// \brief Highlight the edges of a strategy on an automaton.
  ///
  /// Pass a negative color to not display the corresponding strategy.
  SPOT_API
  twa_graph_ptr highlight_strategy(twa_graph_ptr& arena,
                                   int player0_color = 5,
                                   int player1_color = 4);

  /// \brief Set the owner for all the states.
  SPOT_API
  void set_state_players(twa_graph_ptr arena, std::vector<bool> owners);
  SPOT_API
  void set_state_players(twa_graph_ptr arena, std::vector<bool>* owners);

  /// \brief Set the owner of a state.
  SPOT_API
  void set_state_player(twa_graph_ptr arena, unsigned state, unsigned owner);

  /// \brief Get the owner of all the state.
  SPOT_API
  const std::vector<bool>& get_state_players(const_twa_graph_ptr arena);

  /// \brief Get the owner of a state.
  SPOT_API
  unsigned get_state_player(const_twa_graph_ptr arena, unsigned state);

  /// \brief Set the output variables
  SPOT_API
  void set_synthesis_outputs(twa_graph_ptr arena, bdd outs);

  /// \brief Get the output variables as conjunction
  /// \Note : Throws exception if named_property is not set
  SPOT_API
  bdd get_synthesis_outputs(twa_graph_ptr arena);
}