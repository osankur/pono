
#pragma once

#include "engines/ic3ia.h"
#include "smt-switch/utils.h"
#include "smt/external_interpolator.h"

namespace pono {

/**
 * This solver only accepts models with Boolean and real variables.
 * Real variables are assumed to be timed automata clocks, and delay transitions
 * are added.
*/
class IC3IAQ : public IC3IA
{
 public:
    IC3IAQ(const Property & p,
        const TransitionSystem & ts,
        const smt::SmtSolver & s,
        PonoOptions opt = PonoOptions());

  virtual ~IC3IAQ() {}

  typedef IC3IA super;
  void reconstruct_trace(const ProofGoal * pg, smt::TermVec & out) override;
  RefineResult refine() override;

  private:
  ExternalInterpolator external_interpolator_;
};

}