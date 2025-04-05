#include <gtest/gtest.h>

#include <optional>
#include <variant>
#include <vector>

#include "syntax/lexer/span.h"
#include "syntax/lexer/token.h"
#include "syntax/lexer/token_kind.h"
#include "syntax/parser/token_source.h"

namespace {
using orion::syntax::Span;
using orion::syntax::Token;
using orion::syntax::TokenKind;
using orion::syntax::TokenSource;

const auto kSpan = Span(0, 0);
constexpr auto kTokenKindPlus = TokenKind::kPlus;
constexpr auto kTokenKindMinus = TokenKind::kMinus;
constexpr auto kTokenKindWhitespace = TokenKind::kWhitespace;
constexpr auto kTokenKindNewline = TokenKind::kNewline;
constexpr auto kTokenKindComment = TokenKind::kComment;

const auto kTokenPlus =
    Token(static_cast<uint16_t>(kTokenKindPlus), kSpan, U"+");

const auto kTokenMinus =
    Token(static_cast<uint16_t>(kTokenKindMinus), kSpan, U"-");

const auto kTokenWhitespace =
    Token(static_cast<uint16_t>(kTokenKindWhitespace), kSpan, U" ");

const auto kTokenNewline =
    Token(static_cast<uint16_t>(kTokenKindNewline), kSpan, U"\n");

const auto kTokenComment =
    Token(static_cast<uint16_t>(kTokenKindComment), kSpan, U"// hello");

const std::vector<Token> kTokens = {kTokenPlus};

const std::vector<Token> kMultipleTokens = {kTokenPlus, kTokenMinus};

const std::vector<Token> kTokensWithWhitespace = {
    kTokenWhitespace, kTokenNewline, kTokenComment, kTokenPlus};

TEST(TokenSourceTest, NextToken) {
  auto source = TokenSource(kTokens);
  EXPECT_EQ(kTokenPlus, source.NextToken());
  EXPECT_EQ(std::nullopt, source.NextToken());
}

TEST(TokenSourceTest, NextTokenWithTrivia) {
  auto source = TokenSource(kTokensWithWhitespace);
  EXPECT_EQ(kTokenPlus, source.NextToken());
  EXPECT_EQ(std::nullopt, source.NextToken());
}

TEST(TokenSourceTest, NextTokenMultipleTokens) {
  auto source = TokenSource(kMultipleTokens);
  EXPECT_EQ(kTokenPlus, source.NextToken());
  EXPECT_EQ(kTokenMinus, source.NextToken());
}

TEST(TokenSourceTest, PeekKind) {
  auto source = TokenSource(kTokens);
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
}

TEST(TokenSourceTest, PeekKindWithTrivia) {
  auto source = TokenSource(kTokensWithWhitespace);
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
}

TEST(TokenSourceTest, PeekToken) {
  auto source = TokenSource(kTokens);
  EXPECT_EQ(kTokenPlus, source.PeekToken());
  EXPECT_EQ(kTokenPlus, source.PeekToken());
}

TEST(TokenSourceTest, PeekTokenWithTrivia) {
  auto source = TokenSource(kTokensWithWhitespace);
  EXPECT_EQ(kTokenPlus, source.PeekToken());
  EXPECT_EQ(kTokenPlus, source.PeekToken());
}
}  // namespace
