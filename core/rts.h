/*********************                                                        */
/*! \file rts.h
** \verbatim
** Top contributors (to current version):
**   Makai Mann
** This file is part of the pono project.
** Copyright (c) 2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved.  See the file LICENSE in the top-level source
** directory for licensing information.\endverbatim
**
** \brief Implements a relational transition system interface
**
**
**/

#pragma once

#include "core/ts.h"

namespace pono {

class RelationalTransitionSystem : public TransitionSystem
{
 public:
  RelationalTransitionSystem() : TransitionSystem(), invariant_(nullptr) {}

  RelationalTransitionSystem(const smt::SmtSolver & s) : TransitionSystem(s), invariant_(s->make_term(true)) {}

  RelationalTransitionSystem(const TransitionSystem & other_ts,
                             smt::TermTranslator & tt)
      : TransitionSystem(other_ts, tt)
  {
  }

  RelationalTransitionSystem(const TransitionSystem & other_ts)
      : TransitionSystem(other_ts)
  {
    functional_ = false;
    deterministic_ = false;
  }

  /* Sets init and trans to the provided values
   * @param init the new initial state constraints (boolean sort)
   * @param trans the new transition relation constraints (boolean sort)
   */
  void set_behavior(const smt::Term & init, const smt::Term & trans);

  /* Sets transition relation to the provided formula
   * @param trans the new transition relation
   */
  void set_trans(const smt::Term & trans);

  /* Add to the transition relation constraints
   * @param constraint new constraint on transition relation
   */
  void constrain_trans(const smt::Term & constraint);

  void constrain_invariant(const smt::Term & constraint);
  const smt::Term invariant();

  private:
  smt::Term invariant_;
};

}  // namespace pono
