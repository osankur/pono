#pragma once
#include "core/ts.h"
#include "core/rts.h"

namespace pono{

enum TimedAutomatonEncoding
{
    NonCompact,
    Compact
};

/**
 * Relational transition relation encoding timed automata semantics.
 * The encoding is built in two steps: first, from the input VMT file,
 * the transition relation of the discrete steps, as well as the location invariants,
 * and urgent location formulas read. Then, encode_timed_automaton_delays builds
 * the full transition relation including delays, and respecting location invariants and urgency.
 * 
 * There are two possible semantics:
 * - non-compact delays: each transition is either a delay, or a discrete step
 * - compact delays: each transition is a discrete step followed by a delay
 * 
 * @see TimedVMTEncoder
*/
class TimedTransitionSystem : public RelationalTransitionSystem {
    public:
    TimedTransitionSystem(const smt::SmtSolver & s) : 
        RelationalTransitionSystem(s),
        solver_(s),
        locinvar_(solver_->make_term(true)),
        urgent_(solver_->make_term(false)),
        encoded_delays_(false),
        has_dummy_init_transitions_(false)
        {
        }
    static const std::string DELAY_VAR_NAME;
    static const std::string DUMMY_INIT_VAR_NAME;

    const smt::UnorderedTermSet & nonclock_vars() {
        return nonclock_vars_;
    }
    const smt::UnorderedTermSet & clock_vars() {
        return clock_vars_;
    }
    void add_clock_var(smt::Term var){
        clock_vars_.insert(var);
    }
    void add_nonclock_var(smt::Term var){
        nonclock_vars_.insert(var);
    }
    const smt::Term & locinvar(){
        return locinvar_;
    }
    const smt::Term & urgent(){
        return urgent_;
    }

    void add_locinvar(const smt::Term & inv){
        locinvar_ = solver_->make_term(smt::And, locinvar_, inv);
    }
    void add_urgent(const smt::Term & u){
        urgent_ = solver_->make_term(smt::Or, urgent(), u);
    }

    /**
     * Redefine the transition relation by adding delays
     * @pre urgent() only contains nonclock variables
     * @pre locinvar() only contains upper bounds on clocks (when put in negation normal form)
     * @pre invar() does not contain clock variables
    */
    void encode_timed_automaton_delays(const TimedAutomatonEncoding & encoding = TimedAutomatonEncoding::Compact);

    protected:
    /**
     * Given trans(C,X, I, C',X'), add a real delay variable, and redefine the trans relation as
     * 
     *    C >= 0 
     * /\ delta >= 0 
     * /\ (urgent -> delta = 0) 
     * /\ locinvar 
     * /\ (trans \/ (C'=C+delta)) 
     * /\ locinvar'
     * 
    */
    void encode_noncompact_delays();

    /* Add trivial edges on init states. This is necessary for the compact encoding since otherwise we cannot start a run with a delay.
     * trans := trans \/ X=X'/\ C=C' /\ init
     */
    void add_dummy_init_transitions();

    /**
     * Given trans(C,X, I, C',X'), add a real delay variable, and redefine the trans relation as
     * 
     * T(C,X, I, C',X') =
     *    C >= 0 
     * /\ delta >= 0 
     * /\ locinvar(C,X)
     * /\ (urgent -> delta = 0) 
     * /\ (trans(C,X,I,C'-delta,X')
     * /\ locinvar(X',C')
     * 
     * Moreover, to allow delays at the first step, we add a dummy init edge (without guard or reset).
     * @pre locinvar only contains upper bounds on clocks (when put in negation normal form)
    */
    void encode_compact_delays();

    const smt::SmtSolver & solver_;
    smt::UnorderedTermSet nonclock_vars_;
    smt::UnorderedTermSet clock_vars_;
    smt::Term locinvar_;
    smt::Term urgent_;
    smt::Term delta_;

    bool encoded_delays_;
    bool has_dummy_init_transitions_;
};
}