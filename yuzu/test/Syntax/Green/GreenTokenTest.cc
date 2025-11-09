#include "yuzu/Syntax/Green/Green.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenToken;
using yuzu::syntax::GreenTokenData;

const auto token = GreenToken(2, U"3");

TEST(GreenTokenTest, GetKind) {
  // token has a kind of 2.
  EXPECT_EQ(2, token.getKind());
}

TEST(GreenTokenTest, GetSource) {
  // token has a source of "3".
  EXPECT_EQ(U"3", token.getSource());
}

TEST(GreenTokenTest, GetWidth) {
  // token has a width of 1.
  EXPECT_EQ(1, token.getWidth());
}

TEST(GreenTokenTest, GetUseCount) {
  // token has 1 usage (this test).
  EXPECT_EQ(1, token.getUseCount());
}

TEST(GreenNodeTest, Equals) {
  const auto tokenCopy = GreenToken(2, U"3");
  EXPECT_TRUE(token == tokenCopy);
}

TEST(GreenNodeTest, NotEquals) {
  const auto differentToken = GreenToken(2, U"4");
  EXPECT_TRUE(token != differentToken);
}

TEST(GreenTokenTest, GreenTokenSizeRequirements) {
  // shared_ptr:
  //   pointer    = 8
  //   ref_count  = 8
  EXPECT_EQ(16, sizeof(GreenToken));
}

TEST(GreenTokenTest, GreenTokenDataSizeRequirements) {
  // kind                 = 2
  // alignment            = 6
  // std::u32string_view  = 16
  EXPECT_EQ(24, sizeof(GreenTokenData));
}
} // namespace
