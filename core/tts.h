#pragma once
#include "core/ts.h"
#include "core/rts.h"

namespace pono{
class TimedTransitionSystem : public RelationalTransitionSystem {
    public:
    TimedTransitionSystem(const smt::SmtSolver & s, const RelationalTransitionSystem & ts) : RelationalTransitionSystem(ts), original_ts_(ts), solver_(s){
        initialize();
    }
    static const std::string GLOBAL_CLOCK_VAR_NAME;

    const smt::UnorderedTermSet & nonclock_vars() {
        return nonclock_vars_;
    }
    const smt::UnorderedTermSet & clock_vars() {
        return clock_vars_;
    }

    private:
    void initialize();
    const TransitionSystem & original_ts_;
    const smt::SmtSolver & solver_;
    smt::UnorderedTermSet nonclock_vars_;
    smt::UnorderedTermSet clock_vars_;
};
}