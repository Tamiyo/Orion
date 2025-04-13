#include <gtest/gtest.h>

#include <optional>
#include <variant>
#include <vector>

#include "syntax/lexer/span.h"
#include "syntax/lexer/token.h"
#include "syntax/parser/token_source.h"

namespace {
using yuzu::syntax::Span;
using yuzu::syntax::Token;
using yuzu::syntax::TokenSource;

enum class TokenKind : uint16_t {
  kPlus,
  kMinus,
  kWhitespace,
  kNewline,
  kComment
};

bool IsTrivia(TokenKind kind) {
  return kind == TokenKind::kWhitespace || kind == TokenKind::kNewline ||
         kind == TokenKind::kComment;
}

const auto kSpan = Span(0, 0);
constexpr auto kTokenKindPlus = TokenKind::kPlus;
constexpr auto kTokenKindMinus = TokenKind::kMinus;
constexpr auto kTokenKindWhitespace = TokenKind::kWhitespace;
constexpr auto kTokenKindNewline = TokenKind::kNewline;
constexpr auto kTokenKindComment = TokenKind::kComment;

const auto kTokenPlus = Token(kTokenKindPlus, kSpan, U"+");
const auto kTokenMinus = Token(kTokenKindMinus, kSpan, U"-");
const auto kTokenWhitespace = Token(kTokenKindWhitespace, kSpan, U" ");
const auto kTokenNewline = Token(kTokenKindNewline, kSpan, U"\n");
const auto kTokenComment = Token(kTokenKindComment, kSpan, U"// hello");

const auto kTokens = std::vector{kTokenPlus};
const auto kMultipleTokens = std::vector{kTokenPlus, kTokenMinus};
const auto kTokensWithWhitespace =
    std::vector{kTokenWhitespace, kTokenNewline, kTokenComment, kTokenPlus};

TEST(TokenSourceTest, NextToken) {
  auto source = TokenSource<TokenKind>(kTokens, IsTrivia);
  EXPECT_EQ(kTokenPlus, source.NextToken());
  EXPECT_EQ(std::nullopt, source.NextToken());
}

TEST(TokenSourceTest, NextTokenWithTrivia) {
  auto source = TokenSource<TokenKind>(kTokensWithWhitespace, IsTrivia);
  EXPECT_EQ(kTokenPlus, source.NextToken());
  EXPECT_EQ(std::nullopt, source.NextToken());
}

TEST(TokenSourceTest, NextTokenMultipleTokens) {
  auto source = TokenSource<TokenKind>(kMultipleTokens, IsTrivia);
  EXPECT_EQ(kTokenPlus, source.NextToken());
  EXPECT_EQ(kTokenMinus, source.NextToken());
}

TEST(TokenSourceTest, PeekKind) {
  auto source = TokenSource<TokenKind>(kTokens, IsTrivia);
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
}

TEST(TokenSourceTest, PeekKindWithTrivia) {
  auto source = TokenSource<TokenKind>(kTokensWithWhitespace, IsTrivia);
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
}

TEST(TokenSourceTest, PeekToken) {
  auto source = TokenSource<TokenKind>(kTokens, IsTrivia);
  EXPECT_EQ(kTokenPlus, source.PeekToken());
  EXPECT_EQ(kTokenPlus, source.PeekToken());
}

TEST(TokenSourceTest, PeekTokenWithTrivia) {
  auto source = TokenSource<TokenKind>(kTokensWithWhitespace, IsTrivia);
  EXPECT_EQ(kTokenPlus, source.PeekToken());
  EXPECT_EQ(kTokenPlus, source.PeekToken());
}
}  // namespace
