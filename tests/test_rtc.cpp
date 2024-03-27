#include <string>
#include <tuple>
#include <vector>

#include "core/tts.h"
#include "engines/kinduction.h"
#include "engines/ic3ia.h"
#include "engines/ic3iartc.h"
#include "frontends/timed_vmt_encoder.h"
#include "frontends/smv_encoder.h"
#include "gtest/gtest.h"
#include "smt/available_solvers.h"
#include "encoders/test_encoder_inputs.h"
#include "modifiers/property_rtc.h"

using namespace pono;
using namespace smt;
using namespace std;

namespace pono_tests {

const unordered_map<string, pono::ProverResult> inputs(
    { 
      { "sample_consistent.smv", pono::ProverResult::TRUE},
      { "sample_inconsistent.smv", pono::ProverResult::FALSE },
      { "sample_consistent_real.vmt", pono::ProverResult::TRUE },
      { "sample_inconsistent_real.vmt", pono::ProverResult::FALSE }
    });

class RTCTests
    : public ::testing::Test,
      public ::testing::WithParamInterface<
          tuple<SolverEnum, pair<const string, ProverResult>>>
{
};

TEST_P(RTCTests, Encode)
{
  SmtSolver s = create_solver_for(get<0>(GetParam()),
                                  IC3IAQ_ENGINE,
                                  false,
                                  false);
  s->set_opt("produce-unsat-assumptions", "true");
  // PONO_SRC_DIR is a macro set using CMake PROJECT_SRC_DIR
  auto benchmark = get<1>(GetParam());
  string filename = STRFY(PONO_SRC_DIR);
  filename += "/samples/rtc/";
  filename += benchmark.first;
  cout << "Reading file: " << filename << endl;
  ProverResult res = benchmark.second;
  RelationalTransitionSystem rts(s);
  if (filename.find(".vmt") != filename.npos) {
    VMTEncoder se(filename, rts);
    Property plain_prop = Property(rts.solver(), se.propvec()[0]);
    Property prop = get_rt_consistency_property(plain_prop, rts, RTConsistencyMode::DYNAMIC);
    IC3IAQ ic3iartc(prop, rts, s);
    res = ic3iartc.prove();
    EXPECT_EQ(res, benchmark.second);    
  } else if (filename.find(".smv") != filename.npos) {
    SMVEncoder se(filename, rts);
    Property plain_prop(rts.solver(), se.propvec()[0]);  
    Property prop = get_rt_consistency_property(plain_prop, rts, RTConsistencyMode::DYNAMIC);
    IC3IAQ ic3iartc(prop, rts, s);
    res = ic3iartc.prove();
    EXPECT_EQ(res, benchmark.second);
  }
  EXPECT_EQ(res, benchmark.second);
}

INSTANTIATE_TEST_SUITE_P(
    TAIC3IATestSuite,
    RTCTests,
    testing::Combine(testing::ValuesIn({CVC5}),
                     testing::ValuesIn(inputs)));

}