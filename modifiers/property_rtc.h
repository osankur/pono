#pragma once

#include "smt/available_solvers.h"
#include "utils/logger.h"
#include "utils/term_analysis.h"
#include "smt-switch/utils.h"
#include "smt-switch/term.h"
#include "core/prop.h"

namespace pono {

    /** \brief Returns property with the following quantified formula:
     * ~(P(X) /\ ∀ Y. ∀I. T(X, I, Y) -> ~P(Y))
     * where P is p.prop().
     */
    Property get_rt_consistency_property(Property & p, const TransitionSystem & ts) {
    const smt::SmtSolver & solver = p.solver();
    // Map nextstatevar to param
    smt::UnorderedTermMap subst;    
    // for (const auto & sv : ts.statevars()) {
    //   std:: cout << "State var: " << sv->to_string() << "\n";
    // }
    // for (const auto & sv : ts.inputvars()) {
    //   std::cout << "Input var: " << sv->to_string() << "\n";
    // }
    // std::cout << "Original prop: " << p.prop() << "\n";

    // state variable to quantified param
    smt::UnorderedTermMap var2param_;
    // quantified param to state var
    smt::UnorderedTermMap param2var_;

    for (const auto & sv : ts.statevars()) {
        const smt::Sort & sort = sv->get_sort();
        smt::Term p = solver->make_param("#" + sv->to_string(), sort);
        var2param_[sv] = p;
        subst[ts.next(sv)] = p;
        param2var_[p] = sv;
    }
    for (const auto & sv : ts.inputvars()) {
        const smt::Sort & sort = sv->get_sort();
        smt::Term p = solver->make_param("#" + sv->to_string(), sort);
        var2param_[sv] = p;
        subst[sv] = p;
        param2var_[p] = sv;
    }
    
    // std::cout << "trans: " << ts.trans() << "\n";
    // transXY = T(X, pI, pY)
    smt::Term transXY = solver->substitute(ts.trans(), subst);
    // std::cout << "transXY: " << transXY << "\n";
    // badY = ~P(pY)
    smt::Term badY = solver->substitute(solver->make_term(smt::Not, p.prop()), var2param_);
    // T(X, pI, pY) -> ~P(pY)
    smt::Term rhs = solver->make_term(smt::Implies, transXY, badY);
    // forall pI, pY. T(X, pI, pY) -> ~P(pY)
    for (auto pv : param2var_) {
        rhs = solver->make_term(smt::Forall, pv.first, rhs);
    }
    // ~(P(X) /\ (forall pI, pY. T(X, pI, pY) -> ~P(pY)))
    smt::Term rtc_prop = solver->make_term(smt::Not, solver->make_term(smt::And, p.prop(), rhs));
    // std::cout << "RTC Prop: " << rtc_prop << "\n";
    return Property(p.solver(), 
            rtc_prop,
            "_rtc_" + p.name()
            );
    }
}