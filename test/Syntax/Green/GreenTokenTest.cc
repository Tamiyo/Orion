#include "Syntax/Green/Green.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenToken;
using yuzu::syntax::GreenTokenData;

const auto Token = GreenToken(2, U"3");

TEST(GreenTokenTest, GetKind) {
  // Token has a kind of 2.
  EXPECT_EQ(2, Token.getKind());
}

TEST(GreenTokenTest, GetSource) {
  // Token has a source of "3".
  EXPECT_EQ(U"3", Token.getSource());
}

TEST(GreenTokenTest, GetWidth) {
  // Token has a width of 1.
  EXPECT_EQ(1, Token.getWidth());
}

TEST(GreenTokenTest, GetUseCount) {
  // Token has 1 usage (this test).
  EXPECT_EQ(1, Token.getUseCount());
}

TEST(GreenNodeTest, Equals) {
  const auto TokenCopy = GreenToken(2, U"3");
  EXPECT_TRUE(Token == TokenCopy);
}

TEST(GreenNodeTest, NotEquals) {
  const auto DifferentToken = GreenToken(2, U"4");
  EXPECT_TRUE(Token != DifferentToken);
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
  // std::u32string_view  = 32
  EXPECT_EQ(40, sizeof(GreenTokenData));
}
} // namespace
