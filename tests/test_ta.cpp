#include <string>
#include <tuple>
#include <vector>

#include "core/tts.h"
#include "engines/kinduction.h"
#include "engines/ic3ia.h"
#include "frontends/timed_vmt_encoder.h"
#include "gtest/gtest.h"
#include "smt/available_solvers.h"
#include "encoders/test_encoder_inputs.h"

using namespace pono;
using namespace smt;
using namespace std;

namespace pono_tests {

const unordered_map<string, pono::ProverResult> ta_inputs(
    { 
      { "test_urgent.vmt", pono::ProverResult::TRUE },      
      { "simple_ta.vmt", pono::ProverResult::TRUE },
      { "simple_ta_2.vmt", pono::ProverResult::FALSE },
      { "rod_2.smv_4.vmt", pono::ProverResult::TRUE }, // rationals *10
      { "railway_gate_continuous.smv_1.vmt", pono::ProverResult::FALSE }, // rationals * 10
      { "csma_02.smv_1.vmt", pono::ProverResult::TRUE },
      { "simple_ta.vmt", pono::ProverResult::TRUE },
      { "lynch_protocol_02.smv_0.vmt", pono::ProverResult::TRUE },
      { "fisher_uppaal_02.smv_0.vmt", pono::ProverResult::TRUE },
      { "simple_ta_3.vmt", pono::ProverResult::TRUE },
      { "simple_ta_2.vmt", pono::ProverResult::FALSE },
      { "simple_ta_unsafe.vmt", pono::ProverResult::FALSE },
      { "critical_region_02.smv_0.vmt", pono::ProverResult::FALSE },
      { "test_urgent_2.vmt", pono::ProverResult::TRUE },
      { "one_loc_safe.vmt", pono::ProverResult::TRUE },
      { "one_loc_unsafe.vmt", pono::ProverResult::FALSE },
      { "simple_ta_2.vmt", pono::ProverResult::FALSE },
      { "bug_compact_lu.vmt", pono::ProverResult::FALSE },
      { "critical_region_debug.vmt", pono::ProverResult::FALSE },
      { "fddi_02.smv_0.vmt", pono::ProverResult::TRUE },
      { "fischer_star_timed_2.smv_0.vmt", pono::ProverResult::TRUE },
      { "fischer_star_timed_2.smv_1.vmt", pono::ProverResult::FALSE },
      { "fischer_star_timed_3.smv_0.vmt", pono::ProverResult::TRUE },
      { "fischer_star_timed_3.smv_1.vmt", pono::ProverResult::FALSE },
      { "fischer_timed_2.smv_7.vmt", pono::ProverResult::TRUE },
      { "fischer_timed_2.smv_8.vmt", pono::ProverResult::FALSE },
      { "fisher_timed_sv_2.smv_0.vmt", pono::ProverResult::FALSE },
      { "gate.smv_0.vmt", pono::ProverResult::TRUE },
      { "train_gate_02.smv_0.vmt", pono::ProverResult::TRUE },
      { "railway_gate_continuous.smv_0.vmt", pono::ProverResult::TRUE }  // rationals * 10      
    });

class TAIC3IATests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          tuple<SolverEnum, pair<const string, ProverResult>>>
{
};

TEST_P(TAIC3IATests, Encode)
{
  // MSAT IC3IA
  SmtSolver s = create_solver(get<0>(GetParam()));
  s->set_opt("incremental", "true");
  s->set_opt("produce-models", "true");
  TimedTransitionSystem rts(s);
  // PONO_SRC_DIR is a macro set using CMake PROJECT_SRC_DIR
  auto benchmark = get<1>(GetParam());
  string filename = STRFY(PONO_SRC_DIR);
  filename += "/samples/rtc/";
  filename += benchmark.first;
  cout << "Reading file: " << filename << endl;
  TimedVMTEncoder se(filename, rts);
  Property prop(rts.solver(), se.propvec()[0]);  
  PonoOptions opts;
  opts.use_external_opensmt_interpolator_ = true;
  IC3IA ic3ia(prop, rts, s, opts);
  ProverResult res = ic3ia.prove();
  EXPECT_EQ(res, benchmark.second);
}

// class TAIC3IAOpenSMTTests
//     : public ::testing::Test,
//       public ::testing::WithParamInterface<
//           tuple<SolverEnum, pair<const string, ProverResult>>>
// {
// };
// INSTANTIATE_TEST_SUITE_P(
//     TAIC3IAOpenSMTTestSuite,
//     TAIC3IATests,
//     testing::Combine(testing::ValuesIn({CVC5}),
//                      testing::ValuesIn(ta_inputs)));


// TEST_P(TAIC3IAOpenSMTTests, Encode)
// {
//   SmtSolver s = create_solver(get<0>(GetParam()));
//   s->set_opt("incremental", "true");
//   s->set_opt("produce-models", "true");
//   TimedTransitionSystem rts(s);
//   // PONO_SRC_DIR is a macro set using CMake PROJECT_SRC_DIR
//   auto benchmark = get<1>(GetParam());
//   string filename = STRFY(PONO_SRC_DIR);
//   filename += "/samples/rtc/";
//   filename += benchmark.first;
//   cout << "Reading file: " << filename << endl;
//   TimedVMTEncoder se(filename, rts);
//   Property prop(rts.solver(), se.propvec()[0]);  
//   PonoOptions opts;
//   opts.use_external_opensmt_interpolator_ = true;
//   IC3IA ic3ia(prop, rts, s, opts);
//   ProverResult res = ic3ia.prove();
//   EXPECT_EQ(res, benchmark.second);
// }

INSTANTIATE_TEST_SUITE_P(
    TAIC3IATestSuite,
    TAIC3IATests,
    testing::Combine(testing::ValuesIn({MSAT}),
                     testing::ValuesIn(ta_inputs)));


class TAKINDTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          tuple<SolverEnum, pair<const string, ProverResult>>>
{
};

TEST_P(TAKINDTests, Encode)
{
  // CVC5 k-Induction
  SmtSolver s = create_solver(get<0>(GetParam()));
  s->set_opt("incremental", "true");
  s->set_opt("produce-models", "true");
  TimedTransitionSystem rts(s);
  // PONO_SRC_DIR is a macro set using CMake PROJECT_SRC_DIR
  auto benchmark = get<1>(GetParam());
  string filename = STRFY(PONO_SRC_DIR);
  filename += "/samples/rtc/";
  filename += benchmark.first;
  cout << "Reading file: " << filename << endl;
  TimedVMTEncoder se(filename, rts);
  Property prop(rts.solver(), se.propvec()[0]);  
  KInduction kind(prop, rts, s);
  ProverResult res = kind.check_until(10);
  // if the result is false, we must have found false
  if (benchmark.second == pono::ProverResult::FALSE){
    EXPECT_EQ(res, benchmark.second);
  } else {
    // otherwise we accept true or unknown
    EXPECT_NE(res, pono::ProverResult::FALSE);
  }
}

INSTANTIATE_TEST_SUITE_P(
    TAKINDTestSuite,
    TAKINDTests,
    testing::Combine(testing::ValuesIn({CVC5}),
                     testing::ValuesIn(ta_inputs)));

}  // namespace pono_tests
