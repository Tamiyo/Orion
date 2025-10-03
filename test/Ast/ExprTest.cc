#include "Ast/Expr.h"

#include "Ast/ExprBuilder.h"
#include "Ast/SyntaxKind.h"
#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <string>
#include <typeinfo>
#include <vector>

namespace {
using yuzu::ast::BinaryExpr;
using yuzu::ast::Expr;
using yuzu::ast::ExprBuilder;
using yuzu::ast::LiteralExpr;
using yuzu::ast::SyntaxKind;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::SyntaxNode;

TEST(ExprTest, CastNoChildren) {
  SyntaxNode Syntax = SyntaxNode::createRoot(
      GreenNode(static_cast<uint16_t>(SyntaxKind::InfixExpr),
                std::vector<GreenElement>()));

  const std::unique_ptr<Expr> Ast = ExprBuilder::tryFrom(std::move(Syntax));
  EXPECT_TRUE(Ast->is<BinaryExpr>());
}

TEST(ExprTest, CastWithChildren) {
  // 2+3
  SyntaxNode Syntax = SyntaxNode::createRoot(GreenNode(
      static_cast<uint16_t>(SyntaxKind::InfixExpr),
      std::vector<GreenElement>{
          // 2
          GreenElement(GreenNode(
              static_cast<uint16_t>(SyntaxKind::LiteralExpr),
              std::vector<GreenElement>{GreenElement(GreenToken(
                  static_cast<uint16_t>(SyntaxKind::Number), U"2"))})),

          // +
          GreenElement(
              GreenToken(static_cast<uint16_t>(SyntaxKind::Plus), U"+")),

          // 3
          GreenElement(GreenNode(
              static_cast<uint16_t>(SyntaxKind::LiteralExpr),
              std::vector<GreenElement>{GreenElement(GreenToken(
                  static_cast<uint16_t>(SyntaxKind::Number), U"3"))})),
      }));

  std::unique_ptr<Expr> Ast = ExprBuilder::tryFrom(std::move(Syntax));
  ASSERT_NE(nullptr, Ast);

  EXPECT_TRUE(Ast->is<BinaryExpr>());

  const std::optional<const BinaryExpr *> AstAsBinary = Ast->tryAs<BinaryExpr>();
  ASSERT_NE(nullptr, AstAsBinary);

  std::unique_ptr<Expr> Lhs = (*AstAsBinary)->getLhs();
  std::unique_ptr<Expr> Rhs = (*AstAsBinary)->getRhs();
  ASSERT_NE(nullptr, Lhs);
  ASSERT_NE(nullptr, Rhs);

  EXPECT_TRUE(Lhs->is<LiteralExpr>());
  EXPECT_TRUE(Rhs->is<LiteralExpr>());

  const std::optional<const LiteralExpr*> LhsAsLiteral = Lhs->tryAs<LiteralExpr>();
  const std::optional<const LiteralExpr*> RhsAsLiteral = Rhs->tryAs<LiteralExpr>();
  ASSERT_NE(nullptr, LhsAsLiteral);
  ASSERT_NE(nullptr, RhsAsLiteral);

  EXPECT_EQ(U"2", (*LhsAsLiteral)->getValue());
  EXPECT_EQ(U"3", (*RhsAsLiteral)->getValue());
}
} // namespace
