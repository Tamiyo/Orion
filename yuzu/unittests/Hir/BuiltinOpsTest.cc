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
    const auto *expr = ctx.getBuilder().makeIntLit(0);
    ctx.getTypeContext().bind(expr, typeFor(k));
    return expr;
  }

  std::array<const Expr *, 2> argsOf(TypeKind a, TypeKind b) {
    return {typedExpr(a), typedExpr(b)};
  }

  const Type *typeFor(TypeKind k) {
    auto &i = ctx.getTypeContext();
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
    case TypeKind::Relation:
    case TypeKind::Struct:
    case TypeKind::Infer:
      // Compound — can't be built from a bare kind; op tests don't use it.
      return nullptr;
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
                                           OpCase{"Div", DivOp::get(), "/"},
                                           OpCase{"Pow", PowOp::get(), "**"}),
                         [](const auto &info) { return info.param.name; });

//===----------------------------------------------------------------------===//
// Logical (`and`, `or`) — operands must both be `bool`; result is `bool`.
//===----------------------------------------------------------------------===//

class LogicalOpsParam : public BuiltinOpsTest,
                        public ::testing::WithParamInterface<OpCase> {};

TEST_P(LogicalOpsParam, BoolAndBool_ReturnsBool_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Bool, TypeKind::Bool);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Bool);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(LogicalOpsParam, OneNonBoolOperand_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Bool);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

TEST_P(LogicalOpsParam, FailureDiagnostic_MentionsOperatorSymbol) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Int32);
  (void)GetParam().op->resolve(args, ctx);
  ASSERT_EQ(diagnostics.getErrorCount(), 1u);
  const auto &msg = diagnostics.getDiagnostics()[0].message;
  EXPECT_NE(msg.find(GetParam().symbol), std::string::npos)
      << "expected `" << GetParam().symbol << "` in diagnostic: " << msg;
}

INSTANTIATE_TEST_SUITE_P(AllLogicalOps, LogicalOpsParam,
                         ::testing::Values(OpCase{"And", AndOp::get(), "and"},
                                           OpCase{"Or", OrOp::get(), "or"}),
                         [](const auto &info) { return info.param.name; });

//===----------------------------------------------------------------------===//
// Membership (`in`, `not in`) — until container types land, the only
// definable form is substring containment (`str in str`).
//===----------------------------------------------------------------------===//

class MembershipOpsParam : public BuiltinOpsTest,
                           public ::testing::WithParamInterface<OpCase> {};

TEST_P(MembershipOpsParam, StrInStr_ReturnsBool_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Str, TypeKind::Str);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Bool);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(MembershipOpsParam, IntInInt_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Int32);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

TEST_P(MembershipOpsParam, StrInInt_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Str, TypeKind::Int32);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

INSTANTIATE_TEST_SUITE_P(
    AllMembershipOps, MembershipOpsParam,
    ::testing::Values(OpCase{"In", InOp::get(), "in"},
                      OpCase{"NotIn", NotInOp::get(), "not in"}),
    [](const auto &info) { return info.param.name; });

//===----------------------------------------------------------------------===//
// Equality (`==`, `!=`) — operands that coerce to a common type are
// comparable; result is always `bool`.
//===----------------------------------------------------------------------===//

class EqualityOpsParam : public BuiltinOpsTest,
                         public ::testing::WithParamInterface<OpCase> {};

TEST_P(EqualityOpsParam, IntEqInt_ReturnsBool_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Int32);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Bool);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(EqualityOpsParam, BoolEqBool_ReturnsBool_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Bool, TypeKind::Bool);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Bool);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(EqualityOpsParam, StrEqStr_ReturnsBool_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Str, TypeKind::Str);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Bool);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(EqualityOpsParam, IntEqFloat_CoercesAndReturnsBool) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Float64);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Bool);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(EqualityOpsParam, IntEqStr_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Str);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

// Mixed signedness refuses to coerce (see TypeCoercion), so even equality
// reports it as inapplicable — matches the arithmetic family's behaviour.
TEST_P(EqualityOpsParam, MixedSignSameWidth_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Int8, TypeKind::UInt8);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

INSTANTIATE_TEST_SUITE_P(AllEqualityOps, EqualityOpsParam,
                         ::testing::Values(OpCase{"Eq", EqOp::get(), "=="},
                                           OpCase{"Neq", NeqOp::get(), "!="}),
                         [](const auto &info) { return info.param.name; });

//===----------------------------------------------------------------------===//
// Ordering (`<`, `<=`, `>`, `>=`) — numeric (coerced) or string operands;
// `bool` has no ordering. Result is `bool`.
//===----------------------------------------------------------------------===//

class OrderingOpsParam : public BuiltinOpsTest,
                         public ::testing::WithParamInterface<OpCase> {};

TEST_P(OrderingOpsParam, IntCmpInt_ReturnsBool_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Int32);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Bool);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(OrderingOpsParam, IntCmpFloat_CoercesAndReturnsBool) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Float32);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Bool);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(OrderingOpsParam, StrCmpStr_ReturnsBool_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Str, TypeKind::Str);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Bool);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(OrderingOpsParam, BoolCmpBool_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Bool, TypeKind::Bool);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

TEST_P(OrderingOpsParam, IntCmpStr_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Str);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

INSTANTIATE_TEST_SUITE_P(AllOrderingOps, OrderingOpsParam,
                         ::testing::Values(OpCase{"Lt", LtOp::get(), "<"},
                                           OpCase{"Lte", LteOp::get(), "<="},
                                           OpCase{"Gt", GtOp::get(), ">"},
                                           OpCase{"Gte", GteOp::get(), ">="}),
                         [](const auto &info) { return info.param.name; });

//===----------------------------------------------------------------------===//
// Bit shifts (`<<`, `>>`) — both operands must be integers; the result
// takes the left operand's type and the two sides are NOT coerced together
// (the right operand is just a shift count).
//===----------------------------------------------------------------------===//

class ShiftOpsParam : public BuiltinOpsTest,
                      public ::testing::WithParamInterface<OpCase> {};

TEST_P(ShiftOpsParam, IntShiftInt_ReturnsLhsType_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Int8);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Int32);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(ShiftOpsParam, UIntShiftUInt_ReturnsLhsType_NoDiagnostic) {
  const auto args = argsOf(TypeKind::UInt16, TypeKind::UInt8);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::UInt16);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

// Mixed signedness is fine for shifts — the right operand is a count,
// not coerced against the left. Confirms the deliberate divergence from
// the arithmetic family's "mixed sign refuses to coerce" rule.
TEST_P(ShiftOpsParam, MixedSignOperands_ReturnsLhsType_NoDiagnostic) {
  const auto args = argsOf(TypeKind::Int8, TypeKind::UInt8);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Int8);
  EXPECT_EQ(diagnostics.getErrorCount(), 0u);
}

TEST_P(ShiftOpsParam, FloatLhs_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Float32, TypeKind::Int32);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

TEST_P(ShiftOpsParam, FloatRhs_ReturnsError_EmitsDiagnostic) {
  const auto args = argsOf(TypeKind::Int32, TypeKind::Float32);
  const auto *result = GetParam().op->resolve(args, ctx);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(result->getKind(), TypeKind::Error);
  EXPECT_EQ(diagnostics.getErrorCount(), 1u);
}

INSTANTIATE_TEST_SUITE_P(
    AllShiftOps, ShiftOpsParam,
    ::testing::Values(OpCase{"ShiftLeft", ShiftLeftOp::get(), "<<"},
                      OpCase{"ShiftRight", ShiftRightOp::get(), ">>"}),
    [](const auto &info) { return info.param.name; });

//===----------------------------------------------------------------------===//
// Op::getName — the polymorphic accessor used by the printer.
//===----------------------------------------------------------------------===//

TEST(BuiltinOpsNameTest, EachSingletonReportsItsClassName) {
  EXPECT_EQ(AddOp::get()->getName(), "Add");
  EXPECT_EQ(SubOp::get()->getName(), "Sub");
  EXPECT_EQ(MulOp::get()->getName(), "Mul");
  EXPECT_EQ(DivOp::get()->getName(), "Div");
  EXPECT_EQ(PowOp::get()->getName(), "Pow");
  EXPECT_EQ(AndOp::get()->getName(), "And");
  EXPECT_EQ(OrOp::get()->getName(), "Or");
  EXPECT_EQ(InOp::get()->getName(), "In");
  EXPECT_EQ(NotInOp::get()->getName(), "NotIn");
  EXPECT_EQ(EqOp::get()->getName(), "Eq");
  EXPECT_EQ(NeqOp::get()->getName(), "Neq");
  EXPECT_EQ(LtOp::get()->getName(), "Lt");
  EXPECT_EQ(LteOp::get()->getName(), "Lte");
  EXPECT_EQ(GtOp::get()->getName(), "Gt");
  EXPECT_EQ(GteOp::get()->getName(), "Gte");
  EXPECT_EQ(ShiftLeftOp::get()->getName(), "ShiftLeft");
  EXPECT_EQ(ShiftRightOp::get()->getName(), "ShiftRight");
}

} // namespace
