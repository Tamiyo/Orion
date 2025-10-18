#include "Syntax/Green/Green.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenElement;

TEST(GreenElementTest, GreenElementSizeRequirements) {
  // std::variant:
  //   shared_ptr:
  //     pointer    = 8
  //     ref_count  = 8
  // index          = 4
  // alignment      = 4
  EXPECT_EQ(24, sizeof(GreenElement));
}
} // namespace
