#include "HirTestUtils.h"

#include "yuzu/Hir/HirPrinter.h"

#include <gtest/gtest.h>

namespace {

using yuzu::hir::HirPrinter;
using yuzu::hir::test::HirFixture;

class HirPrinterTest : public HirFixture {};

TEST_F(HirPrinterTest, LiteralExpr) {
  const auto *expr = lowerExpr(U"42");
  ASSERT_NE(expr, nullptr);
  EXPECT_EQ("IntLit value=42", HirPrinter::printToString(expr));
}

TEST_F(HirPrinterTest, BinaryExprIncludesOp) {
  const auto *expr = lowerExpr(U"1 + 2");
  ASSERT_NE(expr, nullptr);
  EXPECT_EQ(R"(BinaryExpr op=Add
  IntLit value=1
  IntLit value=2)",
            HirPrinter::printToString(expr));
}

TEST_F(HirPrinterTest, NestedBinaryExpr) {
  // `1 + 2 * 3` parses as `1 + (2 * 3)`.
  const auto *expr = lowerExpr(U"1 + 2 * 3");
  ASSERT_NE(expr, nullptr);
  EXPECT_EQ(R"(BinaryExpr op=Add
  IntLit value=1
  BinaryExpr op=Mul
    IntLit value=2
    IntLit value=3)",
            HirPrinter::printToString(expr));
}

TEST_F(HirPrinterTest, RootWithStmts) {
  const auto *root = lowerRoot(U"1\n2 + 3\n");
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(R"(Root
  ExprStmt
    IntLit value=1
  ExprStmt
    BinaryExpr op=Add
      IntLit value=2
      IntLit value=3)",
            HirPrinter::printToString(root));
}

TEST_F(HirPrinterTest, EmptyRoot) {
  const auto *root = lowerRoot(U"");
  ASSERT_NE(root, nullptr);
  EXPECT_EQ("Root", HirPrinter::printToString(root));
}

} // namespace
