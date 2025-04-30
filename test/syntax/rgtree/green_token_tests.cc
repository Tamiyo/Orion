#include <gtest/gtest.h>

#include <cstdint>

#include "syntax/rgtree/green.h"

namespace {
using yuzu::syntax::GreenToken;
using yuzu::syntax::GreenTokenData;

enum class SyntaxKind : uint16_t {};

TEST(GreenTokenTest, GreenTokenSizeRequirements) {
  // shared_ptr:
  //   pointer   = 8
  //   ref_count = 8
  EXPECT_EQ(16, sizeof(GreenToken<SyntaxKind>));
}

TEST(GreenTokenTest, GreenTokenDataSizeRequirements) {
  // kind                 = 2
  // alignment            = 6
  // std::u32string_view  = 16
  EXPECT_EQ(24, sizeof(GreenTokenData<SyntaxKind>));
}
};  // namespace
