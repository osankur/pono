#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h> 
#include <sys/types.h>
#include <sstream>
#include <regex>
#include <filesystem>
#include <iostream>
#include <fstream>
#include "smt-switch/utils.h"
#include "smt/opensmt_interpolator.h"
#include "smt-switch/term_translator.h"

namespace pono{

std::string OpenSMTInterpolator::clean_up_smt_query(std::string query) const {
  // terms output by smt-switch contain expressions (to_real N)
  // which are used to convert integers to reals but these are not accepted by opensmt in QF_LRA.
  // we will get rid of these, as well as the negative form e.g. (to_real (- 1)).
  std::string s = query;
  std::regex to_real_regex(".*(\\(to_real (\\(?[ \\-0-9]+\\)?)\\)).*");
  std::smatch matches;
  while(std::regex_search(s, matches, to_real_regex)){
    assert(matches.size() >= 3);
    std::string to_real_n = matches[1].str(); // the whole (to_real *) string
    // replace ( by \\(, and ) by \\)
    std::string::size_type pos = 0u;
    while((pos = to_real_n.find("(", pos)) != std::string::npos){
      to_real_n.replace(pos, 1, "\\(");
      pos += 2;
    }
    pos = 0u;
    while((pos = to_real_n.find(")", pos)) != std::string::npos){
      to_real_n.replace(pos, 1, "\\)");
      pos += 2;
    }

    std::regex to_real_n_re(to_real_n);
    std::string n = matches[2].str(); // the number inside, possibly with a minus
    s = std::regex_replace(s, to_real_n_re, n);
  }
  return s;
}

void OpenSMTInterpolator::execute_query(std::string query, smt::TermVec & outInterpolants) const {
  const char * executable = "opensmt";
  // write query to file
  std::string smt2filename = std::tmpnam(nullptr) + std::string(".smt2");
  std::ofstream outfile;
  outfile.open(smt2filename);
  outfile << query;
  outfile.close();

  std::array<char, 128> buffer;
  std::string result;
  std::string cmd = std::string(executable) + " " + smt2filename;
  // std::cout << "executing " << cmd <<"\n";
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
      throw std::runtime_error("popen() failed!");
  }
  // std::cout << "program launched\n";
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
      result += buffer.data();
  }
  std::cout << "opensmt returned: " << result << "\n";

  // check if unsat
  // Typical output looks like
  // unsat
  // ((<= 6 y@1)
  // (or (<= 9 (+ y@2 (* -1 x@2))) (<= 11 (+ y@2 (* -1 x@2)))))
  //
  // so we check if the output starts with unsat and filter it out
  auto it = result.find("unsat");
  if (it == result.npos){
    throw std::runtime_error("opensmt query failed with output: " + result);
  } else {
    result = result.substr(it+5, result.size()-5);
    std::cout << "interpolants: " << result << "\n";
  }
}

smt::Result OpenSMTInterpolator::get_interpolant(const smt::Term & A, const smt::Term & B, smt::Term & out_I) {
  return smt::Result(smt::UNKNOWN);
}

smt::Result OpenSMTInterpolator::get_sequence_interpolants(const smt::TermVec & formulae, smt::TermVec & outInterpolants) {
  std::stringstream ss;
  smt::UnorderedTermSet symbols;
  for (auto f : formulae){
    get_free_symbols(f, symbols);
  }
  // todo check that all symbols are either real or bool
  ss << "(set-option :produce-interpolants true)\n";
  ss << "(set-logic QF_LRA)\n";
  for (auto f : symbols){
    ss << "(declare-fun " << f << " () " << f->get_sort() << ")\n";    
  }
  for (size_t i = 0; i < formulae.size(); i++){
    ss << "(assert (! " << formulae[i] <<" :named f" << i << " ))\n";
  }
  ss << "(check-sat)\n";
  ss << "(get-interpolants ";
  for (size_t i = 0; i < formulae.size(); i++){
    ss << "f" << i << " ";
  }
  ss << ")\n(exit)\n";
  std::cout << ss.str();

  std::string s = clean_up_smt_query(ss.str()); 
  std::cout << s <<"\n" ;
  execute_query(s, outInterpolants);
  return smt::Result(smt::UNKNOWN);  
}


}