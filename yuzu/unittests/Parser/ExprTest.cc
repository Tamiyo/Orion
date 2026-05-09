#include "yuzu/Parser/Grammar/Expr.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"
#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/SyntaxKind.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <ostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace {
using yuzu::lexer::Lexer;
using yuzu::lexer::Token;
using yuzu::lexer::TokenKind;
using yuzu::parser::Event;
using yuzu::parser::Marker;
using yuzu::parser::Parser;
using yuzu::parser::TokenSink;
using yuzu::parser::TokenSource;
using yuzu::parser::parseExpr;
using yuzu::syntax::GreenChild;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using AstKind = yuzu::ast::SyntaxKind;

// Tokens and nodes both end up in the green tree as `uint16_t`, but they live
// in different enum value spaces (lexer::TokenKind vs ast::SyntaxKind), so a
// tag is needed to disambiguate them for assertions.
struct ChildKind {
  bool isNode;
  uint16_t value;

  bool operator==(const ChildKind &other) const {
    return isNode == other.isNode && value == other.value;
  }
};

ChildKind node(AstKind kind) {
  return ChildKind{.isNode = true, .value = static_cast<uint16_t>(kind)};
}

ChildKind tok(TokenKind kind) {
  return ChildKind{.isNode = false, .value = static_cast<uint16_t>(kind)};
}

[[maybe_unused]] std::ostream &operator<<(std::ostream &os,
                                          const ChildKind &kind) {
  return os << (kind.isNode ? "node(" : "tok(") << kind.value << ")";
}

ChildKind kindOf(const GreenChild &child) {
  if (const auto *n = std::get_if<GreenNode>(&child.element)) {
    return ChildKind{.isNode = true,
                     .value = static_cast<uint16_t>(n->getKind())};
  }
  return ChildKind{
      .isNode = false,
      .value = static_cast<uint16_t>(
          std::get<GreenToken>(child.element).getKind())};
}

bool isTrivia(const ChildKind &kind) {
  if (kind.isNode) {
    return false;
  }
  const auto tk = static_cast<TokenKind>(kind.value);
  return tk == TokenKind::Space || tk == TokenKind::Newline ||
         tk == TokenKind::Comment;
}

std::vector<ChildKind> nonTriviaKinds(const GreenNode &n) {
  std::vector<ChildKind> kinds;
  for (const GreenChild &child : n.getChildren()) {
    const ChildKind kind = kindOf(child);
    if (isTrivia(kind)) {
      continue;
    }
    kinds.emplace_back(kind);
  }
  return kinds;
}

// Returns the n-th non-trivia child *node* of `parent`. Aborts on overrun or
// if a token appears where a node was expected.
const GreenNode &nthNode(const GreenNode &parent, size_t n) {
  for (const GreenChild &child : parent.getChildren()) {
    const ChildKind kind = kindOf(child);
    if (isTrivia(kind)) {
      continue;
    }
    const auto *inner = std::get_if<GreenNode>(&child.element);
    if (inner == nullptr) {
      continue;
    }
    if (n == 0) {
      return *inner;
    }
    --n;
  }
  std::abort();
}

// Drive the full lex → parse → sink pipeline for a single expression. The
// outer Expr marker guarantees the green tree always has a root, even when
// parseExpr fails on an empty or malformed input.
TokenSink::Result parseToTree(const std::u32string &source) {
  auto lexer = Lexer(source);
  const std::vector<Token> tokens = lexer.getTokens();

  auto parser = Parser(TokenSource(tokens));
  const Marker root = parser.start();
  parseExpr(parser);
  auto _ = parser.complete(root, AstKind::Expr);

  const std::vector<Event> events = std::move(parser).finish();
  auto sink = TokenSink(tokens, events);
  return sink.finish();
}

// Skip the synthetic Expr root the test harness wraps every parse in.
const GreenNode &innerExpr(const TokenSink::Result &result) {
  return nthNode(result.green, 0);
}

TEST(ExprTest, ParsesNumberLiteral) {
  const auto result = parseToTree(U"42");

  EXPECT_TRUE(result.errors.empty());
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(AstKind::LiteralExpr),
            innerExpr(result).getKind());
  EXPECT_EQ((std::vector<ChildKind>{tok(TokenKind::Number)}),
            nonTriviaKinds(innerExpr(result)));
}

TEST(ExprTest, ParsesIdentifier) {
  const auto result = parseToTree(U"foo");

  EXPECT_TRUE(result.errors.empty());
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(AstKind::Ident),
            innerExpr(result).getKind());
  EXPECT_EQ((std::vector<ChildKind>{tok(TokenKind::Ident)}),
            nonTriviaKinds(innerExpr(result)));
}

TEST(ExprTest, ParsesAddition) {
  const auto result = parseToTree(U"1 + 2");

  EXPECT_TRUE(result.errors.empty());
  const GreenNode &expr = innerExpr(result);
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(AstKind::BinaryExpr),
            expr.getKind());
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Plus),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(expr));
}

TEST(ExprTest, ParsesSubtraction) {
  const auto result = parseToTree(U"5 - 3");

  EXPECT_TRUE(result.errors.empty());
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Minus),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(innerExpr(result)));
}

TEST(ExprTest, ParsesMultiplication) {
  const auto result = parseToTree(U"2 * 3");

  EXPECT_TRUE(result.errors.empty());
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Star),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(innerExpr(result)));
}

TEST(ExprTest, ParsesDivision) {
  const auto result = parseToTree(U"8 / 2");

  EXPECT_TRUE(result.errors.empty());
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Slash),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(innerExpr(result)));
}

// Verifies that `1 + 2 * 3` parses as `1 + (2 * 3)`.
TEST(ExprTest, MultiplicationBindsTighterThanAddition) {
  const auto result = parseToTree(U"1 + 2 * 3");

  EXPECT_TRUE(result.errors.empty());
  const GreenNode &expr = innerExpr(result);
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Plus),
                                    node(AstKind::BinaryExpr)}),
            nonTriviaKinds(expr));

  const GreenNode &rhs = nthNode(expr, 1);
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Star),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(rhs));
}

// Verifies that `1 * 2 + 3` parses as `(1 * 2) + 3`.
TEST(ExprTest, AdditionFlowsAroundMultiplication) {
  const auto result = parseToTree(U"1 * 2 + 3");

  EXPECT_TRUE(result.errors.empty());
  const GreenNode &expr = innerExpr(result);
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::BinaryExpr),
                                    tok(TokenKind::Plus),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(expr));

  const GreenNode &lhs = nthNode(expr, 0);
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Star),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(lhs));
}

// Verifies that `1 + 2 + 3` parses as `(1 + 2) + 3`.
TEST(ExprTest, AdditionIsLeftAssociative) {
  const auto result = parseToTree(U"1 + 2 + 3");

  EXPECT_TRUE(result.errors.empty());
  const GreenNode &expr = innerExpr(result);
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::BinaryExpr),
                                    tok(TokenKind::Plus),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(expr));

  const GreenNode &lhs = nthNode(expr, 0);
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Plus),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(lhs));
}

// Verifies that `1 * 2 * 3` parses as `(1 * 2) * 3`.
TEST(ExprTest, MultiplicationIsLeftAssociative) {
  const auto result = parseToTree(U"1 * 2 * 3");

  EXPECT_TRUE(result.errors.empty());
  const GreenNode &expr = innerExpr(result);
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::BinaryExpr),
                                    tok(TokenKind::Star),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(expr));

  const GreenNode &lhs = nthNode(expr, 0);
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Star),
                                    node(AstKind::LiteralExpr)}),
            nonTriviaKinds(lhs));
}

// `1 +` should still wrap the LHS+operator in a BinaryExpr and report one
// error for the missing RHS — partial trees are how the parser stays
// recoverable.
TEST(ExprTest, RecoversFromMissingRhs) {
  const auto result = parseToTree(U"1 +");

  EXPECT_EQ(1u, result.errors.size());
  const GreenNode &expr = innerExpr(result);
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(AstKind::BinaryExpr),
            expr.getKind());
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::LiteralExpr),
                                    tok(TokenKind::Plus)}),
            nonTriviaKinds(expr));
}

// `+ 1` has no LHS, so parseLhs reports an error and injects an Error node
// containing the unexpected token.
TEST(ExprTest, RecoversFromMissingLhs) {
  const auto result = parseToTree(U"+ 1");

  EXPECT_EQ(1u, result.errors.size());
  EXPECT_EQ((std::vector<ChildKind>{node(AstKind::Error)}),
            nonTriviaKinds(result.green));
}
} // namespace
