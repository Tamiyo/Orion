#include "yuzu/Ast/Ast.h"

#include "yuzu/Syntax/Green/Green.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

namespace {
using yuzu::ast::AstChildren;
using yuzu::ast::BinaryExpr;
using yuzu::ast::Expr;
using yuzu::ast::SyntaxKind;
using yuzu::ast::SyntaxNode;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;

GreenNode makeBinaryExpr() {
  return GreenNode::create(static_cast<uint16_t>(SyntaxKind::BinaryExpr),
                           std::vector<GreenElement>());
}

GreenNode makeLiteralExpr() {
  return GreenNode::create(static_cast<uint16_t>(SyntaxKind::LiteralExpr),
                           std::vector<GreenElement>());
}

GreenNode makeParenExpr() {
  return GreenNode::create(static_cast<uint16_t>(SyntaxKind::ParenExpr),
                           std::vector<GreenElement>());
}

TEST(AstIteratorTest, EmptyParentYieldsNoChildren) {
  const auto green = GreenNode::create(static_cast<uint16_t>(SyntaxKind::Stmt),
                                       std::vector<GreenElement>());
  const auto root = SyntaxNode::createRoot(green);
  const AstChildren<BinaryExpr> children(root.getChildren());

  EXPECT_EQ(children.begin(), children.end());
}

TEST(AstIteratorTest, NonMatchingChildrenAreSkippedEntirely) {
  const auto green = GreenNode::create(
      static_cast<uint16_t>(SyntaxKind::Stmt),
      std::vector<GreenElement>{makeLiteralExpr(), makeLiteralExpr()});
  const auto root = SyntaxNode::createRoot(green);
  const AstChildren<BinaryExpr> children(root.getChildren());

  EXPECT_EQ(children.begin(), children.end());
}

TEST(AstIteratorTest, OnlyMatchingChildrenAreYielded) {
  const auto green = GreenNode::create(
      static_cast<uint16_t>(SyntaxKind::Stmt),
      std::vector<GreenElement>{makeBinaryExpr(), makeLiteralExpr(),
                                makeBinaryExpr()});
  const auto root = SyntaxNode::createRoot(green);
  const AstChildren<BinaryExpr> children(root.getChildren());

  int count = 0;
  for (const auto &child : children) {
    (void)child;
    ++count;
  }
  EXPECT_EQ(count, 2);
}

TEST(AstIteratorTest, VariantBaseMatchesAllConcreteVariants) {
  const auto green = GreenNode::create(
      static_cast<uint16_t>(SyntaxKind::Stmt),
      std::vector<GreenElement>{makeBinaryExpr(), makeLiteralExpr(),
                                makeParenExpr()});
  const auto root = SyntaxNode::createRoot(green);
  const AstChildren<Expr> children(root.getChildren());

  int count = 0;
  for (const auto &child : children) {
    (void)child;
    ++count;
  }
  EXPECT_EQ(count, 3);
}

TEST(AstIteratorTest, BeginSkipsLeadingNonMatches) {
  const auto green = GreenNode::create(
      static_cast<uint16_t>(SyntaxKind::Stmt),
      std::vector<GreenElement>{makeLiteralExpr(), makeLiteralExpr(),
                                makeBinaryExpr()});
  const auto root = SyntaxNode::createRoot(green);
  const AstChildren<BinaryExpr> children(root.getChildren());

  int count = 0;
  for (const auto &child : children) {
    (void)child;
    ++count;
  }
  EXPECT_EQ(count, 1);
}

TEST(AstIteratorTest, IncrementSkipsInterleavedNonMatches) {
  const auto green = GreenNode::create(
      static_cast<uint16_t>(SyntaxKind::Stmt),
      std::vector<GreenElement>{makeBinaryExpr(), makeLiteralExpr(),
                                makeBinaryExpr(), makeLiteralExpr()});
  const auto root = SyntaxNode::createRoot(green);
  const AstChildren<BinaryExpr> children(root.getChildren());

  int count = 0;
  for (const auto &child : children) {
    (void)child;
    ++count;
  }
  EXPECT_EQ(count, 2);
}
} // namespace
