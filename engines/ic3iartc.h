
#pragma once

#include "engines/ic3ia.h"
#include "smt-switch/utils.h"
#include "smt/opensmt_interpolator.h"

namespace pono {

/**
 * This solver only accepts models with Boolean and real variables.
 * Real variables are assumed to be timed automata clocks, and delay transitions
 * are added.
*/
class IC3IARTC : public IC3IA
{
 public:
    IC3IARTC(const Property & p,
        const TransitionSystem & ts,
        const smt::SmtSolver & s,
        PonoOptions opt = PonoOptions());

  virtual ~IC3IARTC() {}

  typedef IC3IA super;
  void reconstruct_trace(const ProofGoal * pg, smt::TermVec & out) override;
  RefineResult refine() override;

  private:
  bool use_opensmt_interpolator_;
  OpenSMTInterpolator openSMTInterpolator_;
};

}