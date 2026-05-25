#include "yuzu/Parser/TokenSource.h"

#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <string_view>

namespace {
using yuzu::lexer::Lexer;
using yuzu::lexer::Range;
using yuzu::lexer::Token;
using yuzu::lexer::TokenKind;
using yuzu::parser::TokenSource;

inline TokenSource createTokenSource(const std::u32string &source) {
  auto lexer = Lexer(source);
  const auto tokens = lexer.getTokens();
  return TokenSource(tokens);
}

TEST(TokenSourceTest, EmptyGetNextTokenReturnsNullopt) {
  auto source = createTokenSource(U"");
  EXPECT_EQ(std::nullopt, source.getNextToken());
}

TEST(TokenSourceTest, EmptyPeekNextTokenReturnsNullopt) {
  auto source = createTokenSource(U"");
  EXPECT_EQ(std::nullopt, source.peekNextKind());
}

TEST(TokenSourceTest, TriviaGetNextTokenReturnsNullopt) {
  auto source = createTokenSource(U" ");
  EXPECT_EQ(std::nullopt, source.getNextToken());
}

TEST(TokenSourceTest, TriviaPeekNextTokenReturnsNullopt) {
  auto source = createTokenSource(U" ");
  EXPECT_EQ(std::nullopt, source.peekNextKind());
}

TEST(TokenSourceTest, TriviaAndIdentPeekNextTokenReturnsToken) {
  const auto identToken = Token(TokenKind::Identifier, U"a", Range{1, 2});
  auto source = createTokenSource(U" a");
  EXPECT_EQ(identToken, source.getNextToken());
}

TEST(TokenSourceTest, TriviaAndIdentPeekNextKindReturnsToken) {
  const auto identToken = Token(TokenKind::Identifier, U"a", Range{1, 2});
  auto source = createTokenSource(U" a");
  EXPECT_EQ(identToken.getKind(), source.peekNextKind());
}

TEST(TokenSourceTest, PeekLastTokenOnEmptyReturnsNullopt) {
  auto source = createTokenSource(U"");
  EXPECT_EQ(std::nullopt, source.peekLastToken());
}

TEST(TokenSourceTest, PeekLastTokenReturnsLastToken) {
  const auto identToken = Token(TokenKind::Identifier, U"b", Range{2, 3});
  auto source = createTokenSource(U"a b");
  EXPECT_EQ(identToken, source.peekLastToken());
}
} // namespace
