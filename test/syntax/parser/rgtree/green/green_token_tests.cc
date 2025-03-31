#include <gtest/gtest.h>

#include "syntax/parser/rgtree/green/green_token.h"

namespace {
TEST(GreenTokenTest, GreenTokenSizeRequirements) {
  // shared_ptr:
  //   pointer   = 8
  //   ref_count = 8
  EXPECT_EQ(16, sizeof(orion::syntax::GreenToken));
}

TEST(GreenTokenTest, GreenTokenDataSizeRequirements) {
  // kind           = 2
  // alignment      = 6
  // std::u32string = 24
  EXPECT_EQ(40, sizeof(orion::syntax::GreenTokenData));
}
};  // namespace
