#include <gtest/gtest.h>

#include <optional>
#include <variant>
#include <vector>

#include "syntax/lexer/span.h"
#include "syntax/lexer/token.h"
#include "syntax/lexer/token_kind.h"
#include "syntax/parser/token_source.h"

namespace {
const auto kSpan = orion::syntax::Span(0, 0);
const auto kTokenKindPlus = orion::syntax::TokenKind::kPlus;
const auto kTokenKindMinus = orion::syntax::TokenKind::kMinus;
const auto kTokenKindWhitespace = orion::syntax::TokenKind::kWhitespace;
const auto kTokenKindNewline = orion::syntax::TokenKind::kNewline;
const auto kTokenKindComment = orion::syntax::TokenKind::kComment;

const auto kTokenPlus =
    orion::syntax::Token(static_cast<uint16_t>(kTokenKindPlus), kSpan, U"+");

const auto kTokenMinus =
    orion::syntax::Token(static_cast<uint16_t>(kTokenKindMinus), kSpan, U"-");

const auto kTokenWhitespace = orion::syntax::Token(
    static_cast<uint16_t>(kTokenKindWhitespace), kSpan, U" ");

const auto kTokenNewline = orion::syntax::Token(
    static_cast<uint16_t>(kTokenKindNewline), kSpan, U"\n");

const auto kTokenComment = orion::syntax::Token(
    static_cast<uint16_t>(kTokenKindComment), kSpan, U"// hello");

const std::vector<orion::syntax::Token> kTokens = {kTokenPlus};

const std::vector<orion::syntax::Token> kMultipleTokens = {kTokenPlus,
                                                           kTokenMinus};

const std::vector<orion::syntax::Token> kTokensWithWhitespace = {
    kTokenWhitespace, kTokenNewline, kTokenComment, kTokenPlus};

TEST(TokenSourceTest, NextToken) {
  auto source = orion::syntax::TokenSource(kTokens);
  EXPECT_EQ(kTokenPlus, source.NextToken());
  EXPECT_EQ(std::nullopt, source.NextToken());
}

TEST(TokenSourceTest, NextTokenWithTrivia) {
  auto source = orion::syntax::TokenSource(kTokensWithWhitespace);
  EXPECT_EQ(kTokenPlus, source.NextToken());
  EXPECT_EQ(std::nullopt, source.NextToken());
}

TEST(TokenSourceTest, NextTokenMultipleTokens) {
  auto source = orion::syntax::TokenSource(kMultipleTokens);
  EXPECT_EQ(kTokenPlus, source.NextToken());
  EXPECT_EQ(kTokenMinus, source.NextToken());
}

TEST(TokenSourceTest, PeekKind) {
  auto source = orion::syntax::TokenSource(kTokens);
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
}

TEST(TokenSourceTest, PeekKindWithTrivia) {
  auto source = orion::syntax::TokenSource(kTokensWithWhitespace);
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
  EXPECT_EQ(kTokenKindPlus, source.PeekKind());
}

TEST(TokenSourceTest, PeekToken) {
  auto source = orion::syntax::TokenSource(kTokens);
  EXPECT_EQ(kTokenPlus, source.PeekToken());
  EXPECT_EQ(kTokenPlus, source.PeekToken());
}

TEST(TokenSourceTest, PeekTokenWithTrivia) {
  auto source = orion::syntax::TokenSource(kTokensWithWhitespace);
  EXPECT_EQ(kTokenPlus, source.PeekToken());
  EXPECT_EQ(kTokenPlus, source.PeekToken());
}
}  // namespace
