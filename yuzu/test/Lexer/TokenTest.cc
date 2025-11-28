#include "gtest/gtest.h"

#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include <string_view>

namespace {
using yuzu::lexer::Range;
using yuzu::lexer::Token;
using yuzu::lexer::TokenKind;

TEST(TokenTest, TokenSizeRequirements) {
  // source         = 16
  // range          = 8
  // kind           = 4
  // padding        = 4
  EXPECT_EQ(32, sizeof(Token));
}

TEST(TokenTest, GetLength) {
  const auto range = Range{.start = 0, .end = 4};
  const auto source = U"hello";
  const auto token = Token(TokenKind::Error, source, range);

  EXPECT_EQ(4, token.getLength());
}
}; // namespace
