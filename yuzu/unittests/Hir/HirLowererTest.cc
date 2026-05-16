#include "HirTestUtils.h"

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Hir/Hir.h"

#include <gtest/gtest.h>

namespace {

using yuzu::diagnostics::Severity;
using yuzu::hir::BinaryExpr;
using yuzu::hir::BinOp;
using yuzu::hir::ExprStmt;
using yuzu::hir::HirKind;
using yuzu::hir::test::HirFixture;

class HirLowererTest : public HirFixture {};

TEST_F(HirLowererTest, LiteralExprLowersToLiteralExpr) {
  const auto *expr = lowerExpr(U"42");
  ASSERT_NE(expr, nullptr);
  EXPECT_EQ(expr->getKind(), HirKind::IntLit);
  EXPECT_FALSE(engine.hasErrors());
}

TEST_F(HirLowererTest, BinaryExprAddLowersWithCorrectOp) {
  const auto *expr = lowerExpr(U"1 + 2");
  ASSERT_NE(expr, nullptr);
  ASSERT_EQ(expr->getKind(), HirKind::BinaryExpr);
  const auto *bin = BinaryExpr::cast(expr);
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Add);
  ASSERT_NE(bin->getLhs(), nullptr);
  ASSERT_NE(bin->getRhs(), nullptr);
  EXPECT_EQ(bin->getLhs()->getKind(), HirKind::IntLit);
  EXPECT_EQ(bin->getRhs()->getKind(), HirKind::IntLit);
  EXPECT_FALSE(engine.hasErrors());
}

TEST_F(HirLowererTest, BinaryExprSubMapsToSub) {
  const auto *bin = BinaryExpr::cast(lowerExpr(U"1 - 2"));
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Sub);
}

TEST_F(HirLowererTest, BinaryExprMulMapsToMul) {
  const auto *bin = BinaryExpr::cast(lowerExpr(U"3 * 4"));
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Mul);
}

TEST_F(HirLowererTest, BinaryExprDivMapsToDiv) {
  const auto *bin = BinaryExpr::cast(lowerExpr(U"8 / 2"));
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Div);
}

TEST_F(HirLowererTest, NestedBinaryExprRecurses) {
  // `1 + 2 * 3` parses as `1 + (2 * 3)` under standard precedence.
  const auto *bin = BinaryExpr::cast(lowerExpr(U"1 + 2 * 3"));
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Add);
  EXPECT_EQ(bin->getLhs()->getKind(), HirKind::IntLit);

  const auto *rhs = BinaryExpr::cast(bin->getRhs());
  ASSERT_NE(rhs, nullptr);
  EXPECT_EQ(rhs->getOp(), BinOp::Mul);
}

TEST_F(HirLowererTest, BindsHirNodesToAstOrigins) {
  // Every HIR node the lowerer builds should have its HirId bound back
  // to the AST view it came from. Walk the lowered tree and confirm
  // each node has a source-map entry whose kind matches the AST.
  const auto *bin = BinaryExpr::cast(lowerExpr(U"1 + 2"));
  ASSERT_NE(bin, nullptr);

  const auto binOrigin = sourceMap.get(bin->getId());
  ASSERT_TRUE(binOrigin.has_value());
  EXPECT_EQ(binOrigin->getKind(), yuzu::ast::SyntaxKind::BinaryExpr);

  const auto lhsOrigin = sourceMap.get(bin->getLhs()->getId());
  ASSERT_TRUE(lhsOrigin.has_value());
  EXPECT_EQ(lhsOrigin->getKind(), yuzu::ast::SyntaxKind::IntLit);

  const auto rhsOrigin = sourceMap.get(bin->getRhs()->getId());
  ASSERT_TRUE(rhsOrigin.has_value());
  EXPECT_EQ(rhsOrigin->getKind(), yuzu::ast::SyntaxKind::IntLit);
}

TEST_F(HirLowererTest, RootWithSingleStmtLowers) {
  const auto *root = lowerRoot(U"42");
  ASSERT_NE(root, nullptr);
  EXPECT_FALSE(engine.hasErrors());

  const auto stmts = root->getStmts();
  ASSERT_EQ(stmts.size(), 1u);
  const auto *stmt = stmts[0];
  ASSERT_EQ(stmt->getKind(), HirKind::ExprStmt);

  const auto *exprStmt = ExprStmt::cast(stmt);
  ASSERT_NE(exprStmt, nullptr);
  ASSERT_NE(exprStmt->getExpr(), nullptr);
  EXPECT_EQ(exprStmt->getExpr()->getKind(), HirKind::IntLit);
}

TEST_F(HirLowererTest, RootWithMultipleStmtsLowers) {
  const auto *root = lowerRoot(U"1\n2 + 3\n");
  ASSERT_NE(root, nullptr);
  EXPECT_FALSE(engine.hasErrors());

  ASSERT_EQ(root->getStmts().size(), 2u);
  EXPECT_EQ(root->getStmts()[0]->getKind(), HirKind::ExprStmt);
  EXPECT_EQ(root->getStmts()[1]->getKind(), HirKind::ExprStmt);

  const auto *second = ExprStmt::cast(root->getStmts()[1]);
  ASSERT_NE(second, nullptr);
  EXPECT_EQ(second->getExpr()->getKind(), HirKind::BinaryExpr);
}

TEST_F(HirLowererTest, ErrorOnHirNodeUsesAstSpanFromSourceMap) {
  // `1 + 2`'s rhs LiteralExpr has AST range 4..5. Emitting via the
  // (HirNode *) overload should land at that range — proving the
  // handler walks the source map rather than falling back to a zero
  // span.
  const auto *bin = BinaryExpr::cast(lowerExpr(U"1 + 2"));
  ASSERT_NE(bin, nullptr);

  const auto sourceId = sources.add("<rebuild>", U"1 + 2");
  yuzu::hir::HirLowerer lowerer(builder, sourceMap, engine, sourceId);
  lowerer.error(bin->getRhs(), "boom").emit();

  ASSERT_FALSE(engine.getDiagnostics().empty());
  const auto &d = engine.getDiagnostics().back();
  EXPECT_EQ(d.severity, Severity::Error);
  EXPECT_EQ(d.message, "boom");
  ASSERT_FALSE(d.labels.empty());
  EXPECT_EQ(d.labels[0].span.start, 4u);
  EXPECT_EQ(d.labels[0].span.end, 5u);
}

TEST_F(HirLowererTest, MissingRhsEmitsRightOperandDiagnostic) {
  // Parser builds a BinaryExpr with no rhs. Lowerer should name the
  // specific missing piece.
  const auto *expr = lowerExpr(U"1 +");
  EXPECT_EQ(expr, nullptr);

  bool foundRhs = false;
  for (const auto &d : engine.getDiagnostics()) {
    if (d.severity == Severity::Error &&
        d.message == "binary expression is missing its right operand") {
      foundRhs = true;
      break;
    }
  }
  EXPECT_TRUE(foundRhs);
}

} // namespace
