#include "yuzu/Hir/Ops/BuiltinOps.h"

#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/Op.h"
#include "yuzu/Hir/Types/Type.h"

#include <gtest/gtest.h>

#include <array>
#include <string>

namespace {

using namespace yuzu::hir;
using yuzu::diagnostics::DiagnosticsEngine;
using yuzu::diagnostics::SourceId;

class BuiltinOpsTest : public ::testing::Test {
protected:
  DiagnosticsEngine diagnostics;
  HirContext ctx{diagnostics, SourceId{}};

  /// Build a typed `Expr` whose `getType()` returns the requested
  /// primitive. Resolves only read `arg->getType()`, so using `IntLit`
  /// as a generic carrier keeps the helper one-liner-sized — the value
  /// payload is irrelevant.
  const Expr *typedExpr(TypeKind k) {
    return ctx.getBuilder().makeIntLit(0, typeFor(k));
  }

  std::array<const Expr *, 2> argsOf(TypeKind a, TypeKind b) {
    return {typedExpr(a), typedExpr(b)};
  }

  const Type *typeFor(TypeKind k) {
    auto &i = ctx.getTypeInterner();
    switch (k) {
    case TypeKind::Int8:
      return i.getInt8();
    case TypeKind::Int16:
      return i.getInt16();
    case TypeKind::Int32:
      return i.getInt32();
    case TypeKind::Int64:
      return i.getInt64();
    case TypeKind::UInt8:
      return i.getUInt8();
    case TypeKind::UInt16:
      return i.getUInt16();
    case TypeKind::UInt32:
      return i.getUInt32();
    case TypeKind::UInt64:
      return i.getUInt64();
    case TypeKind::Float32:
      return i.getFloat32();
    case TypeKind::Float64:
      return i.getFloat64();
    case TypeKind::Bool:
      return i.getBool();
    case TypeKind::Str:
      return i.getStr();
    case TypeKind::Error:
      return i.getError();
    }
    return nullptr;
  }
};

//===----------------------------------------------------------------------===//
// AddOp — covers the per-op special case (`str + str -> str`) and the
// shared numeric-coercion path.
//===----------------------------------------------------------------------===//

TEST_F(BuiltinOpsTest, AddOp_IntPlusInt_ReturnsCoercedInt_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Int8, TypeKind::Int32);
  const auto *result = AddOp::get()->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Int32);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_F(BuiltinOpsTest, AddOp_StrPlusStr_ReturnsStr_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Str, TypeKind::Str);
  const auto *result = AddOp::get()->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Str);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_F(BuiltinOpsTest, AddOp_IntPlusStr_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Int64, TypeKind::Str);
  const auto *result = AddOp::get()->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

//===----------------------------------------------------------------------===//
// SubOp — sibling to Add minus the `str + str` special case; confirms the
// special case is Add-only.
//===----------------------------------------------------------------------===//

TEST_F(BuiltinOpsTest, SubOp_StrPlusStr_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Str, TypeKind::Str);
  const auto *result = SubOp::get()->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

//===----------------------------------------------------------------------===//
// Cross-op coverage — the shared numeric-coercion + diagnostic-on-failure
// shape is identical across the four arithmetic ops, so parameterize.
//===----------------------------------------------------------------------===//

struct OpCase {
  std::string name;
  const Op *op;
  std::string symbol;
};

class BuiltinOpsParam : public BuiltinOpsTest,
                        public ::testing::WithParamInterface<OpCase> {};

TEST_P(BuiltinOpsParam, IntPlusFloat_PromotesToFloat) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Float32);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Float32);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(BuiltinOpsParam, MixedSignSameWidth_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Int8, TypeKind::UInt8);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

TEST_P(BuiltinOpsParam, BoolPlusInt_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Bool, TypeKind::Int64);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

// The diagnostic for a failing resolve must mention the operator's
// symbol so the user can find the offending `+` / `-` / `*` / `/` in
// source. Substring-only because the prose around it is allowed to
// evolve.
TEST_P(BuiltinOpsParam, FailureDiagnostic_MentionsOperatorSymbol) {
  const auto args = argsOf(TypeKind::Bool, TypeKind::Int64);
  (void)GetParam().op->resolve(args, ctx);
  ASSERT_EQ(diagnostics.getErrorCount(), 1u);
  const auto &msg = diagnostics.getDiagnostics()[0].message;
  EXPECT_NE(msg.find(GetParam().symbol), std::string::npos)
      << "expected `" << GetParam().symbol << "` in diagnostic: " << msg;
}

INSTANTIATE_TEST_SUITE_P(AllArithmeticOps, BuiltinOpsParam,
                         ::testing::Values(OpCase{"Add", AddOp::get(), "+"},
                                           OpCase{"Sub", SubOp::get(), "-"},
                                           OpCase{"Mul", MulOp::get(), "*"},
                                           OpCase{"Div", DivOp::get(), "/"}),
                         [](const auto &info) { return info.param.name; });

//===----------------------------------------------------------------------===//
// Op::getName — the polymorphic accessor used by the printer.
//===----------------------------------------------------------------------===//

TEST(BuiltinOpsNameTest, EachSingletonReportsItsClassName) {
  EXPECT_EQ(AddOp::get()->getName(), "Add");
  EXPECT_EQ(SubOp::get()->getName(), "Sub");
  EXPECT_EQ(MulOp::get()->getName(), "Mul");
  EXPECT_EQ(DivOp::get()->getName(), "Div");
}

} // namespace
