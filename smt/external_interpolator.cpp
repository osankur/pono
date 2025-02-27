#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h> 
#include <sys/types.h>
#include <sstream>
#include <regex>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <iterator>
#include "utils/logger.h"
#include "smt-switch/utils.h"
#include "smt/external_interpolator.h"
#include "smt-switch/term_translator.h"
#include "smt-switch/smtlib_reader.h"

namespace pono{

std::string ExternalInterpolator::clean_negative_numbers(std::string query) {
  // sed -r 's/-([0-9]+)/(- \1)/g'
  // std::regex negnumber(".*(-([0-9]+)).*");
  // std::smatch matches;
  // while(std::regex_search(query, matches, negnumber)){
  //   assert(matches.size() >= 3);
  //   std::string whole_number = matches[1].str(); // the whole -N string
  //   std::string inside_of_number = matches[2].str(); // just N
  //   std::regex whole_number_reg(whole_number);
  //   query = std::regex_replace(query, whole_number_reg, "(- " + inside_of_number +")");
  // }
  std::string result;
  std::array<char, 128> buffer;

  std::string tmpfilename = std::tmpnam(nullptr);
  std::ofstream outfile(tmpfilename);
  outfile << query;
  outfile.close();

  std::string cmd = "sed -r 's/-([0-9]+)/(- \\1)/g' " + tmpfilename;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
      throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
      result += buffer.data();
  }
  return result;
}

std::string ExternalInterpolator::remove_to_real(std::string query) {
  // terms output by smt-switch contain expressions (to_real N)
  // which are used to convert integers to reals but these are not accepted by opensmt in QF_LRA.
  // we will get rid of these, as well as the negative form e.g. (to_real (- 1)).
  // remove (to_real *) for nonnegative numbers:
  //   sed -r 's/\(to_real\s*([0-9]*)\)/\1/g'
  // remove them for negative numbers:
  //   sed -r 's/\(to_real (\([- 0-9]*\))\)/\1/g'
  std::string result;
  std::array<char, 128> buffer;

  std::string tmpfilename = std::tmpnam(nullptr);
  std::ofstream outfile(tmpfilename);
  outfile << query;
  outfile.close();

  std::string cmd = "sed -r 's/\\(to_real\\s*([0-9]*)\\)/\\1/g' " + tmpfilename + "| sed -r 's/\\(to_real (\\([- 0-9]*\\))\\)/\\1/g' ";
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
      throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
      result += buffer.data();
  }
  return result;
}

std::string ExternalInterpolator::remove_false(std::string query) {
  // remove all occurrences of 'false':
  //   sed -r 's/false//g'
  std::string result;
  std::array<char, 128> buffer;

  std::string tmpfilename = std::tmpnam(nullptr);
  std::ofstream outfile(tmpfilename);
  outfile << query;
  outfile.close();

  std::string cmd = "sed -r 's/false//g' " + tmpfilename;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
      throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
      result += buffer.data();
  }
  return result;
}


std::string ExternalInterpolator::insert_to_real(std::string query) {
  std::string result;
  std::array<char, 128> buffer;

  std::string tmpfilename = std::tmpnam(nullptr);
  std::ofstream outfile(tmpfilename);
  outfile << query;
  outfile.close();

  std::string cmd = "sed -r 's/ ([0-9]+)(\\.0)?/ (to_real \\1)/g' " + tmpfilename;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
      throw std::runtime_error("popen() failed!");
  }
  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
      result += buffer.data();
  }
  return result;
}


smt::Result ExternalInterpolator::execute_query(std::string & query, const smt::UnorderedTermSet & symbols, smt::TermVec & outInterpolants) {
  // write query to file
  std::string smt2filename = std::tmpnam(nullptr) + std::string(".smt2");
  std::ofstream outfile;
  outfile.open(smt2filename);
  outfile << query;
  outfile.close();
  logger.log(2, "Wrote seq interpolant query to file: " + smt2filename);
  std::cout.flush();
  std::cerr.flush();
  std::array<char, 128> buffer;
  std::string result;
  std::string cmd = executable() + " " + smt2filename;
  std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
  if (!pipe) {
      throw std::runtime_error("popen() failed!");
  }

  while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
      result += buffer.data();
  }
  logger.log(2, (executable() + (" returned: " + result)));

  // check if unsat
  // Typical output looks like
  // unsat
  // ((<= 6 y@1)
  // (or (<= 9 (+ y@2 (* -1 x@2))) (<= 11 (+ y@2 (* -1 x@2)))))
  //
  // so we check if the output starts with unsat and filter it out
  auto it = result.find("unsat");
  if (it == result.npos){
    if (result.find("sat") != result.npos){
      return smt::Result(smt::SAT);
    } else throw std::runtime_error(executable() + " query failed with output: " + result);
  } else {
    // Add parantheses around negative numbers (opensmt prints these without parantheses)
    //    -N -> (- N)
    result = clean_negative_numbers(result);
    auto first_open_par = result.find_first_of("(");
    auto last_close_par = result.find_last_of(")");
    result = result.substr(first_open_par+1, last_close_par-first_open_par-1);
    result = remove_false(result);
    // std::cout << "Extracted interp: " << result << "\n";

    // make a conjunction out of the list of interpolants
    result = "(and true " + result + ")";
    // and put the formula in a define-fun with attribute interpolant as follows
    //     (define-fun inter() Bool (! FORMULA :interpolant true))
    result = "(define-fun inter() Bool (! " + result + " :interpolant true))";
    std::string tmpfilename = std::tmpnam(nullptr) + std::string(".smt2");
    std::ofstream outfile(tmpfilename);
    for (auto f : symbols){
      outfile << "(declare-fun " << f << " () " << f->get_sort() << ")\n";
    }
    // insert back to_real around all integers otherwise Pono's parser will assume these are of bitvector sort
    outfile << insert_to_real(result) << "\n";
    outfile.close();
    logger.log(2, "Wrote interpolant to file: " + tmpfilename);

    // We create a fresh temporary solver to read back the produced file as a workaround
    smt::SmtSolver tmp_solver = create_solver(smt::CVC5);
    ExternalInterpolator::InterpolantReader interpolant_reader(tmpfilename, tmp_solver);
    logger.log(2, ("parsed interpolant: " + interpolant_reader.get_interpolant()->to_string()));
    // and then transfer the formula back to original_interpolator
    smt::Term conjunctive = to_original_interpolator_.transfer_term(interpolant_reader.get_interpolant());
    logger.log(2, ("transferred interpolant: "));
    logger.log(2, conjunctive->to_string());

    // We return a single formula which is the conjunction of all interpolants;
    // but it's ok since the refiner then just uses the predicates appearing in these
    outInterpolants.push_back(conjunctive);
  }
  return smt::Result(smt::UNSAT);
}

smt::Result ExternalInterpolator::get_interpolant(const smt::Term & A, const smt::Term & B, smt::Term & out_I) {
  std::stringstream ss;
  smt::UnorderedTermSet symbols;
  get_free_symbols(A, symbols);
  get_free_symbols(B, symbols);
  // todo check that all symbols are either real or bool
  ss << "(set-option :produce-interpolants true)\n";
  ss << "(set-logic QF_LRA)\n";
  for (auto f : symbols){
    ss << "(declare-fun " << f << " () " << f->get_sort() << ")\n";    
  }
  ss << "(assert (! " << A <<" :named fa ))\n";
  ss << "(assert (! " << B <<" :named fb ))\n";
  ss << "(check-sat)\n";
  ss << "(get-interpolant fa fb)";
  ss << ")\n(exit)\n";
  std::string s = remove_to_real(ss.str()); 
  smt::TermVec outInterpolants;
  smt::Result r = execute_query(s, symbols, outInterpolants);
  if (r == smt::UNSAT && outInterpolants.size() > 0){
    out_I = outInterpolants.front();
  } else {
    throw PonoException("Call to external interpolator failed");
  }
  return r;
}

smt::Result ExternalInterpolator::get_sequence_interpolants(const smt::TermVec & formulae, smt::TermVec & outInterpolants) {
  if (externalInterpolator_ == ExternalInterpolatorEnum::Z3){
    // todo implement here an ad-hoc computation of sequence interpolants
    throw PonoException("Z3 does not support sequence interpolants");
  } else {
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
    std::string s = remove_to_real(ss.str()); 
    return execute_query(s, symbols, outInterpolants);
  }
}
}