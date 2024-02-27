/*********************                                                        */
/*! \file vmt_encoder.cpp
** \verbatim
** Top contributors (to current version):
**   Makai Mann
** This file is part of the pono project.
** Copyright (c) 2019 by the authors listed in the file AUTHORS
** in the top-level source directory) and their institutional affiliations.
** All rights reserved.  See the file LICENSE in the top-level source
** directory for licensing information.\endverbatim
**
** \brief Frontend for the Verification Modulo Theories (VMT) format.
**        See https://vmt-lib.fbk.eu/ for more information.
**
**
**/

#include "frontends/timed_vmt_encoder.h"
#include "utils/logger.h"

using namespace smt;
using namespace std;

namespace pono {

TimedVMTEncoder::TimedVMTEncoder(std::string filename,
                                 TimedTransitionSystem & tts)
    : smt::SmtLibReader(tts.get_solver()), filename_(filename), tts_(tts)
{
  set_logic_all();
  int res = parse(filename_);
  assert(!res);  // 0 means success
  tts.encode_timed_automaton_delays();
}

void TimedVMTEncoder::new_symbol(const std::string & name,
                                 const smt::Sort & sort)
{
  smt::SmtLibReader::new_symbol(name, sort);
  if (sort->get_sort_kind() != FUNCTION) {
    // treat as an input variable until given :next
    tts_.add_inputvar(lookup_symbol(name));
  }
}

void TimedVMTEncoder::term_attribute(const Term & term,
                                     const string & keyword,
                                     const string & value)
{
  // std::cout << "term: " << term << "\nkeyword: " << keyword << "\nvalue: " << value << "\n\n";
  if (keyword == "next") {
    Term next_var = lookup_symbol(value);
    if (!next_var) {
      // undeclared next var
      // make a new one
      Sort sort = term->get_sort();
      new_symbol(value, sort);
      next_var = lookup_symbol(value);
    }
    assert(next_var);
    tts_.add_statevar(term, next_var);
    tts_.add_nonclock_var(term);
  } else if (keyword == "init") {
    tts_.constrain_init(term);
  } else if (keyword == "trans") {
    tts_.constrain_trans(term);
  } else if (keyword == "invar") {
    tts_.constrain_init(term);
    tts_.constrain_trans(term);
    tts_.constrain_trans(tts_.next(term));
    tts_.constrain_invariant(term);
  } else if (keyword == "invar-property") {
    propvec_.push_back(term);
  } else if (keyword == "locinvar") {
    tts_.add_locinvar(term);
  } else if (keyword == "urgent") {
    tts_.add_urgent(term);
  } else if (keyword == "nextclock") {
    Term next_var = lookup_symbol(value);
    if (!next_var) {
      // undeclared next var
      // make a new one
      Sort sort = term->get_sort();
      new_symbol(value, sort);
      next_var = lookup_symbol(value);
    }
    assert(next_var);
    tts_.add_statevar(term, next_var);
    tts_.add_clock_var(term);
  } else {
    throw PonoException("Unhandled VMT attribute -- :" + keyword + " " + value);
  }
}

}  // namespace pono
