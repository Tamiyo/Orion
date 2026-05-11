#include "HirTestUtils.h"

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Hir/Hir.h"

#include <gtest/gtest.h>

namespace {

using yuzu::diagnostics::Severity;
using yuzu::hir::BinaryExpr;
using yuzu::hir::BinOp;
using yuzu::hir::HirKind;
using yuzu::hir::LiteralExpr;
using yuzu::hir::test::HirFixture;

class LowererTest : public HirFixture {};

TEST_F(LowererTest, LiteralExprLowersToLiteralExpr) {
  const auto *expr = lowerExpr(U"42");
  ASSERT_NE(expr, nullptr);
  EXPECT_EQ(expr->getKind(), HirKind::LiteralExpr);
  EXPECT_FALSE(engine.hasErrors());
}

TEST_F(LowererTest, BinaryExprAddLowersWithCorrectOp) {
  const auto *expr = lowerExpr(U"1 + 2");
  ASSERT_NE(expr, nullptr);
  ASSERT_EQ(expr->getKind(), HirKind::BinaryExpr);
  const auto *bin = BinaryExpr::cast(expr);
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Add);
  ASSERT_NE(bin->getLhs(), nullptr);
  ASSERT_NE(bin->getRhs(), nullptr);
  EXPECT_EQ(bin->getLhs()->getKind(), HirKind::LiteralExpr);
  EXPECT_EQ(bin->getRhs()->getKind(), HirKind::LiteralExpr);
  EXPECT_FALSE(engine.hasErrors());
}

TEST_F(LowererTest, BinaryExprSubMapsToSub) {
  const auto *bin = BinaryExpr::cast(lowerExpr(U"1 - 2"));
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Sub);
}

TEST_F(LowererTest, BinaryExprMulMapsToMul) {
  const auto *bin = BinaryExpr::cast(lowerExpr(U"3 * 4"));
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Mul);
}

TEST_F(LowererTest, BinaryExprDivMapsToDiv) {
  const auto *bin = BinaryExpr::cast(lowerExpr(U"8 / 2"));
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Div);
}

TEST_F(LowererTest, NestedBinaryExprRecurses) {
  // `1 + 2 * 3` parses as `1 + (2 * 3)` under standard precedence.
  const auto *bin = BinaryExpr::cast(lowerExpr(U"1 + 2 * 3"));
  ASSERT_NE(bin, nullptr);
  EXPECT_EQ(bin->getOp(), BinOp::Add);
  EXPECT_EQ(bin->getLhs()->getKind(), HirKind::LiteralExpr);

  const auto *rhs = BinaryExpr::cast(bin->getRhs());
  ASSERT_NE(rhs, nullptr);
  EXPECT_EQ(rhs->getOp(), BinOp::Mul);
}

TEST_F(LowererTest, IncompleteBinaryExprEmitsDiagnosticAndReturnsNull) {
  // Missing rhs: parser still builds a BinaryExpr node with a missing
  // operand. Lowerer should catch this and report it.
  const auto *expr = lowerExpr(U"1 +");
  EXPECT_EQ(expr, nullptr);

  bool foundIncomplete = false;
  for (const auto &d : engine.getDiagnostics()) {
    if (d.severity == Severity::Error &&
        d.message == "incomplete binary expression") {
      foundIncomplete = true;
      break;
    }
  }
  EXPECT_TRUE(foundIncomplete);
}

} // namespace
