#include <gtest/gtest.h>

#include "syntax/parser/rgtree/green/green_token.h"

namespace {
using orion::syntax::GreenToken;
using orion::syntax::GreenTokenData;

TEST(GreenTokenTest, GreenTokenSizeRequirements) {
  // shared_ptr:
  //   pointer   = 8
  //   ref_count = 8
  EXPECT_EQ(16, sizeof(GreenToken));
}

TEST(GreenTokenTest, GreenTokenDataSizeRequirements) {
  // kind                 = 2
  // alignment            = 6
  // std::u32string_view  = 16
  EXPECT_EQ(24, sizeof(GreenTokenData));
}
};  // namespace
