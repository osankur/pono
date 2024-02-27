#include <string>
#include <tuple>
#include <vector>

#include "core/tts.h"
#include "engines/kinduction.h"
#include "frontends/timed_vmt_encoder.h"
#include "gtest/gtest.h"
#include "smt/available_solvers.h"
#include "encoders/test_encoder_inputs.h"

using namespace pono;
using namespace smt;
using namespace std;

namespace pono_tests {

const unordered_map<string, pono::ProverResult> ta_inputs(
    { { "simple_ta.vmt", pono::ProverResult::TRUE },
      { "simple_ta_2.vmt", pono::ProverResult::FALSE } });

class TATests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          tuple<SolverEnum, pair<const string, ProverResult>>>
{
};

TEST_P(TATests, Encode)
{
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
  EXPECT_EQ(res, benchmark.second);
}

INSTANTIATE_TEST_SUITE_P(
    ParamTATests,
    TATests,
    testing::Combine(testing::ValuesIn({CVC5}),
                     // from test_encoder_inputs.h
                     testing::ValuesIn(ta_inputs)));

}  // namespace pono_tests
