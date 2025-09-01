#include "syntax/green/green_element.h"

#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenElementData;

TEST(GreenElementTest, GreenElementSizeRequirements) {
  // std::shared_ptr:
  //  pointer   = 8
  //  ref_count = 8
  EXPECT_EQ(16, sizeof(GreenElement));
}

TEST(GreenElementTest, GreenElementDataSizeRequirements) {
  // std::variant:
  //   shared_ptr:
  //     pointer    = 8
  //     ref_count  = 8
  // index          = 4
  // alignment      = 4
  EXPECT_EQ(24, sizeof(GreenElementData));
}
};  // namespace
