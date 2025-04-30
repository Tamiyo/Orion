#include <gtest/gtest.h>

#define APPROVALS_GOOGLETEST_EXISTING_MAIN
#include "ApprovalTests/ApprovalTests.hpp"

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);

  ApprovalTests::initializeApprovalTestsForGoogleTests();

  const auto& subdirectory =
      ApprovalTests::Approvals::useApprovalsSubdirectory("approvals");

  const auto& _ =
      ApprovalTests::GoogleConfiguration::addIgnorableTestCaseNameSuffix(
          "Fixture");

  return RUN_ALL_TESTS();
}
