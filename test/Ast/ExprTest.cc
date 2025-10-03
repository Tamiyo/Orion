// #include "Ast/Expr.h"

// #include "Ast/SyntaxKind.h"
// #include "Syntax/Green/Green.h"
// #include "Syntax/Syntax.h"

// #include <gtest/gtest.h>

// #include <cstdint>
// #include <optional>
// #include <string>
// #include <typeinfo>
// #include <vector>

// namespace {
// using yuzu::ast::BinaryExpr;
// using yuzu::ast::Expr;
// using yuzu::ast::ExprKind;
// using yuzu::ast::LiteralExpr;
// using yuzu::ast::SyntaxKind;
// using yuzu::syntax::GreenElement;
// using yuzu::syntax::GreenNode;
// using yuzu::syntax::GreenToken;
// using yuzu::syntax::SyntaxNode;

// TEST(ExprTest, CastNoChildren) {
//   const auto Syntax = SyntaxNode::createRoot(
//       GreenNode(static_cast<uint16_t>(SyntaxKind::InfixExpr),
//                 std::vector<GreenElement>()));

//   const std::optional<Expr> Ast = Expr::tryFrom(Syntax);
//   EXPECT_EQ(ExprKind::BinaryExpr, Ast->getKind());
// }

// TEST(ExprTest, CastWithChildren) {
//   // 2+3
//   const auto Syntax = SyntaxNode::createRoot(GreenNode(
//       static_cast<uint16_t>(SyntaxKind::InfixExpr),
//       std::vector<GreenElement>{
//           // 2
//           GreenElement(GreenNode(
//               static_cast<uint16_t>(SyntaxKind::LiteralExpr),
//               std::vector<GreenElement>{GreenElement(GreenToken(
//                   static_cast<uint16_t>(SyntaxKind::Number), U"2"))})),

//           // +
//           GreenElement(
//               GreenToken(static_cast<uint16_t>(SyntaxKind::Plus), U"+")),

//           // 3
//           GreenElement(GreenNode(
//               static_cast<uint16_t>(SyntaxKind::LiteralExpr),
//               std::vector<GreenElement>{GreenElement(GreenToken(
//                   static_cast<uint16_t>(SyntaxKind::Number), U"3"))})),
//       }));

//   const std::optional<Expr> Ast = Expr::tryFrom(Syntax);
//   ASSERT_TRUE(Ast.has_value());

//   EXPECT_EQ(ExprKind::BinaryExpr, Ast->getKind());

//   const std::optional<BinaryExpr> AstAsBinary = Ast->tryAs<BinaryExpr>();
//   ASSERT_TRUE(AstAsBinary.has_value());

//   const std::optional<Expr> Lhs = AstAsBinary->getLhs();
//   const std::optional<Expr> Rhs = AstAsBinary->getRhs();
//   ASSERT_TRUE(Lhs.has_value());
//   ASSERT_TRUE(Rhs.has_value());

//   EXPECT_EQ(ExprKind::LiteralExpr, Lhs->getKind());
//   EXPECT_EQ(ExprKind::LiteralExpr, Rhs->getKind());

//   const std::optional<LiteralExpr> LhsAsLiteral = Lhs->tryAs<LiteralExpr>();
//   const std::optional<LiteralExpr> RhsAsLiteral = Rhs->tryAs<LiteralExpr>();
//   ASSERT_TRUE(LhsAsLiteral.has_value());
//   ASSERT_TRUE(RhsAsLiteral.has_value());

//   EXPECT_EQ(U"2", LhsAsLiteral->getValue());
//   EXPECT_EQ(U"3", RhsAsLiteral->getValue());
// }
// } // namespace
