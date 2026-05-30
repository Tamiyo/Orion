#include "yuzu/Compiler/CompilePipeline.h"

#include <llvm/Support/raw_ostream.h>

#include <gtest/gtest.h>

#include <string>

namespace {

yuzu::CompileOptions opts(llvm::raw_ostream &os) {
  return yuzu::CompileOptions{
      .out = os,
      .execute = false,
      .debugLexer = false,
      .debugAst = false,
      .debugHir = true,
      .debugMlir = false,
  };
}

TEST(CompilePipelineTest, OnePlusTwoLowersToTypedCall) {
  // `1 + 2`: both operands are int64; the lowerer resolves the call
  // through `AddOp::resolve`, so the resulting CallExpr is typed as
  // int64 and the operands appear as int64 literals.
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 + 2", opts(os));

  EXPECT_NE(out.find("=== hir ==="), std::string::npos) << out;
  EXPECT_NE(out.find("CallExpr : int64"), std::string::npos) << out;
  EXPECT_NE(out.find("value=1"), std::string::npos) << out;
  EXPECT_NE(out.find("value=2"), std::string::npos) << out;
}

TEST(CompilePipelineTest, NestedPrecedenceLowersToNestedCall) {
  // `1 + 2 * 3` parses as `1 + (2 * 3)`. The HIR dump should show two
  // nested `CallExpr` nodes — the outer add and the inner multiply.
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 + 2 * 3", opts(os));

  const auto outer = out.find("CallExpr : int64");
  ASSERT_NE(outer, std::string::npos) << out;
  const auto inner = out.find("CallExpr : int64", outer + 1);
  EXPECT_NE(inner, std::string::npos)
      << "expected two CallExpr nodes for the nested precedence:\n"
      << out;
  EXPECT_NE(out.find("value=3"), std::string::npos) << out;
}

TEST(CompilePipelineTest, MixedNumericCoercesToFloat64) {
  // `1 + 2.0` mixes int64 and float64. `coerceTypes` picks float64
  // as the common type via the numeric promotion rank, so the
  // CallExpr is typed as float64 even though one operand is an
  // IntLit.
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 + 2.0", opts(os));

  EXPECT_NE(out.find("CallExpr : float64"), std::string::npos) << out;
  EXPECT_NE(out.find("IntLit : int64"), std::string::npos) << out;
  EXPECT_NE(out.find("FloatLit : float64"), std::string::npos) << out;
}

TEST(CompilePipelineTest, ParenGroupingFlipsAssociativity) {
  // `(1 + 2) * 3` forces the add to evaluate first — the inverse of
  // `1 + 2 * 3`. The HIR should show Mul as the outer call with Add
  // nested inside. This also locks in the lowering of `ParenExpr`
  // (regression: without the wrapping node, paren tokens leaked into
  // the outer `BinaryExpr` and lowering reported "binary expression is
  // missing its operator").
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"(1 + 2) * 3", opts(os));

  EXPECT_EQ(out.find("error:"), std::string::npos)
      << "paren grouping must not emit a diagnostic:\n"
      << out;
  const auto mul = out.find("op=Mul");
  const auto add = out.find("op=Add");
  ASSERT_NE(mul, std::string::npos) << out;
  ASSERT_NE(add, std::string::npos) << out;
  EXPECT_LT(mul, add) << "expected Mul (outer) before Add (inner):\n" << out;
}

TEST(CompilePipelineTest, IncompleteBinaryEmitsErrorAndDropsCall) {
  // A binary expression missing its rhs is rejected during lowering.
  // The pipeline emits an error diagnostic and the resulting HIR has
  // no `CallExpr` — only an empty `Root` since the broken statement is
  // dropped.
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 +", opts(os));

  EXPECT_NE(out.find("error:"), std::string::npos)
      << "expected an error diagnostic for the incomplete binary:\n"
      << out;
  EXPECT_EQ(out.find("CallExpr"), std::string::npos)
      << "incomplete binary should not produce a CallExpr in HIR:\n"
      << out;
}

} // namespace
