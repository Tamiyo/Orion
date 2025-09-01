#include "syntax/green/green_token.h"

#include <gtest/gtest.h>

#include "syntax/syntax_kind.h"

namespace {
using yuzu::syntax::GreenToken;
using yuzu::syntax::GreenTokenData;
using yuzu::syntax::SyntaxKind;

TEST(GreenTokenTest, GreenTokenSizeRequirements) {
  // std::shared_ptr:
  //  pointer   = 8
  //  ref_count = 8
  EXPECT_EQ(16, sizeof(GreenToken));
}

TEST(GreenTokenTest, GreenTokenDataSizeRequirements) {
  // kind                 = 2
  // alignment            = 6
  // std::u32string_view  = 32
  EXPECT_EQ(40, sizeof(GreenTokenData));
}
};  // namespace
