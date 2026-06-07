#include "yuzu/Hir/Types/TypeCoercion.h"

#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Adjustment.h"
#include "yuzu/Hir/Types/Type.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace {

using namespace yuzu::hir;
using yuzu::diagnostics::DiagnosticsEngine;
using yuzu::diagnostics::SourceId;

const Type *typeFor(const TypeFactory &typeFactory, TypeKind k) {
  switch (k) {
  case TypeKind::Int8:
    return typeFactory.getInt8Type();
  case TypeKind::Int16:
    return typeFactory.getInt16Type();
  case TypeKind::Int32:
    return typeFactory.getInt32Type();
  case TypeKind::Int64:
    return typeFactory.getInt64Type();
  case TypeKind::UInt8:
    return typeFactory.getUInt8Type();
  case TypeKind::UInt16:
    return typeFactory.getUInt16Type();
  case TypeKind::UInt32:
    return typeFactory.getUInt32Type();
  case TypeKind::UInt64:
    return typeFactory.getUInt64Type();
  case TypeKind::Float32:
    return typeFactory.getFloat32Type();
  case TypeKind::Float64:
    return typeFactory.getFloat64Type();
  case TypeKind::Bool:
    return typeFactory.getBoolType();
  case TypeKind::Str:
    return typeFactory.getStrType();
  case TypeKind::Unit:
    return typeFactory.getUnitType();
  case TypeKind::Relation:
  case TypeKind::Struct:
  case TypeKind::Func:
  case TypeKind::TypeParam:
  case TypeKind::Infer:
    // Compound — can't be built from a bare kind; coercion tests don't use it.
    return nullptr;
  case TypeKind::Error:
    return typeFactory.getErrorType();
  }
  return nullptr;
}

class TypeCoercionFixture {
protected:
  DiagnosticsEngine diagnostics;
  HirContext ctx{diagnostics, SourceId{}};

  /// Build a typed `Expr` whose `getType()` returns the requested
  /// primitive. `coerceTypes` only looks at `getType()` and `getId()`,
  /// so `IntLit` makes a fine generic carrier — the value payload is
  /// irrelevant.
  const Expr *typedExpr(TypeKind k) {
    const auto *expr = ctx.getBuilder().makeIntLit(0);
    ctx.getTypeContext().bind(
        expr, typeFor(ctx.getTypeContext().getTypeFactory(), k));
    return expr;
  }
};

//===----------------------------------------------------------------------===//
// Result-type matrix — parameterized over (lhs, rhs) → expected.
//===----------------------------------------------------------------------===//

struct CoercionCase {
  std::string name;
  TypeKind a;
  TypeKind b;
  std::optional<TypeKind> expected;
};

class TypeCoercionTest : public TypeCoercionFixture,
                         public ::testing::TestWithParam<CoercionCase> {};

TEST_P(TypeCoercionTest, Coerces) {
  const auto &c = GetParam();
  const auto *lhs = typedExpr(c.a);
  const auto *rhs = typedExpr(c.b);
  const auto *result = coerceTypes(lhs, rhs, ctx);

  if (!c.expected) {
    EXPECT_EQ(result, nullptr)
        << "expected refusal for " << asString(c.a) << " + " << asString(c.b);
    return;
  }

  ASSERT_NE(result, nullptr) << "expected " << asString(*c.expected) << " for "
                             << asString(c.a) << " + " << asString(c.b);
  EXPECT_EQ(result->getKind(), *c.expected);
}

// Identity: every kind coerces with itself to itself, including the
// non-numeric ones (`bool`, `str`, `<error>`) — that's the first check
// in `coerceTypes`.
INSTANTIATE_TEST_SUITE_P(
    Identity, TypeCoercionTest,
    ::testing::Values(CoercionCase{"Int8WithInt8", TypeKind::Int8,
                                   TypeKind::Int8, TypeKind::Int8},
                      CoercionCase{"UInt32WithUInt32", TypeKind::UInt32,
                                   TypeKind::UInt32, TypeKind::UInt32},
                      CoercionCase{"Float64WithFloat64", TypeKind::Float64,
                                   TypeKind::Float64, TypeKind::Float64},
                      CoercionCase{"BoolWithBool", TypeKind::Bool,
                                   TypeKind::Bool, TypeKind::Bool},
                      CoercionCase{"StrWithStr", TypeKind::Str, TypeKind::Str,
                                   TypeKind::Str},
                      CoercionCase{"ErrorWithError", TypeKind::Error,
                                   TypeKind::Error, TypeKind::Error}),
    [](const auto &info) { return info.param.name; });

// Same-sign integer widening picks the higher-ranked side.
INSTANTIATE_TEST_SUITE_P(
    IntegerWidening, TypeCoercionTest,
    ::testing::Values(CoercionCase{"SignedWidens", TypeKind::Int8,
                                   TypeKind::Int64, TypeKind::Int64},
                      CoercionCase{"SignedWidensReversed", TypeKind::Int64,
                                   TypeKind::Int8, TypeKind::Int64},
                      CoercionCase{"UnsignedWidens", TypeKind::UInt8,
                                   TypeKind::UInt32, TypeKind::UInt32},
                      CoercionCase{"UnsignedWidensReversed", TypeKind::UInt32,
                                   TypeKind::UInt8, TypeKind::UInt32}),
    [](const auto &info) { return info.param.name; });

// Mixed signedness is refused regardless of rank — caller must insert an
// explicit cast.
INSTANTIATE_TEST_SUITE_P(
    MixedSignedness, TypeCoercionTest,
    ::testing::Values(CoercionCase{"SameWidthRefused", TypeKind::Int32,
                                   TypeKind::UInt32, std::nullopt},
                      CoercionCase{"DifferentWidthRefused", TypeKind::Int8,
                                   TypeKind::UInt16, std::nullopt},
                      CoercionCase{"DifferentWidthRefusedReversed",
                                   TypeKind::UInt8, TypeKind::Int16,
                                   std::nullopt}),
    [](const auto &info) { return info.param.name; });

// A float on either side wins the family; rank decides within floats.
INSTANTIATE_TEST_SUITE_P(
    FloatPromotion, TypeCoercionTest,
    ::testing::Values(CoercionCase{"FloatWidens", TypeKind::Float32,
                                   TypeKind::Float64, TypeKind::Float64},
                      CoercionCase{"IntPromotesToFloat", TypeKind::Int32,
                                   TypeKind::Float32, TypeKind::Float32},
                      CoercionCase{"UIntPromotesToFloat", TypeKind::UInt64,
                                   TypeKind::Float64, TypeKind::Float64},
                      CoercionCase{"IntPromotesEvenWhenLargerRank",
                                   TypeKind::Int64, TypeKind::Float32,
                                   TypeKind::Float32}),
    [](const auto &info) { return info.param.name; });

// Once any operand is non-numeric (and the kinds aren't identical), the
// coercion refuses — there's no concat/promotion path for `str`, `bool`,
// or the typed `<error>`.
INSTANTIATE_TEST_SUITE_P(
    NonNumeric, TypeCoercionTest,
    ::testing::Values(CoercionCase{"StrPlusInt64Refused", TypeKind::Str,
                                   TypeKind::Int64, std::nullopt},
                      CoercionCase{"BoolPlusInt64Refused", TypeKind::Bool,
                                   TypeKind::Int64, std::nullopt},
                      CoercionCase{"ErrorPlusInt64Refused", TypeKind::Error,
                                   TypeKind::Int64, std::nullopt},
                      CoercionCase{"StrPlusBoolRefused", TypeKind::Str,
                                   TypeKind::Bool, std::nullopt}),
    [](const auto &info) { return info.param.name; });

//===----------------------------------------------------------------------===//
// Adjustment side-effect tests — the contract is that the narrower
// operand gets a cast adjustment to the wider type, and the wider
// operand is left alone. No adjustments are recorded when the kinds
// already match or when coercion is refused.
//===----------------------------------------------------------------------===//

class TypeCoercionAdjustmentTest : public TypeCoercionFixture,
                                   public ::testing::Test {};

TEST_F(TypeCoercionAdjustmentTest, NarrowerSignedOperandGetsCastToWider) {
  const auto *narrow = typedExpr(TypeKind::Int8);
  const auto *wide = typedExpr(TypeKind::Int32);
  (void)coerceTypes(narrow, wide, ctx);

  const auto *narrowAdj = ctx.getAdjustments().get(narrow->getId());
  ASSERT_NE(narrowAdj, nullptr) << "expected an adjustment on the narrow side";
  EXPECT_EQ(narrowAdj->kind, AdjustmentKind::Cast);
  EXPECT_EQ(narrowAdj->target->getKind(), TypeKind::Int32);

  EXPECT_EQ(ctx.getAdjustments().get(wide->getId()), nullptr)
      << "wider operand should not be adjusted";
}

TEST_F(TypeCoercionAdjustmentTest,
       AdjustmentBindsToNarrowSideRegardlessOfOrder) {
  // Argument order shouldn't change which operand carries the cast —
  // it's always the narrower one.
  const auto *wide = typedExpr(TypeKind::UInt64);
  const auto *narrow = typedExpr(TypeKind::UInt8);
  (void)coerceTypes(wide, narrow, ctx);

  EXPECT_EQ(ctx.getAdjustments().get(wide->getId()), nullptr);
  const auto *narrowAdj = ctx.getAdjustments().get(narrow->getId());
  ASSERT_NE(narrowAdj, nullptr);
  EXPECT_EQ(narrowAdj->target->getKind(), TypeKind::UInt64);
}

TEST_F(TypeCoercionAdjustmentTest, IntPromotedToFloatRecordsCastOnIntOperand) {
  const auto *intOperand = typedExpr(TypeKind::Int64);
  const auto *floatOperand = typedExpr(TypeKind::Float32);
  (void)coerceTypes(intOperand, floatOperand, ctx);

  const auto *intAdj = ctx.getAdjustments().get(intOperand->getId());
  ASSERT_NE(intAdj, nullptr) << "int operand should be cast to the float type";
  EXPECT_EQ(intAdj->target->getKind(), TypeKind::Float32);
  EXPECT_EQ(ctx.getAdjustments().get(floatOperand->getId()), nullptr);
}

TEST_F(TypeCoercionAdjustmentTest, IdenticalTypesRecordNoAdjustment) {
  const auto *a = typedExpr(TypeKind::Int64);
  const auto *b = typedExpr(TypeKind::Int64);
  (void)coerceTypes(a, b, ctx);

  EXPECT_EQ(ctx.getAdjustments().get(a->getId()), nullptr);
  EXPECT_EQ(ctx.getAdjustments().get(b->getId()), nullptr);
}

TEST_F(TypeCoercionAdjustmentTest, RefusedMixedSignednessRecordsNoAdjustment) {
  const auto *a = typedExpr(TypeKind::Int32);
  const auto *b = typedExpr(TypeKind::UInt32);
  const auto *result = coerceTypes(a, b, ctx);

  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(ctx.getAdjustments().get(a->getId()), nullptr);
  EXPECT_EQ(ctx.getAdjustments().get(b->getId()), nullptr);
}

TEST_F(TypeCoercionAdjustmentTest, RefusedNonNumericRecordsNoAdjustment) {
  const auto *a = typedExpr(TypeKind::Str);
  const auto *b = typedExpr(TypeKind::Int64);
  const auto *result = coerceTypes(a, b, ctx);

  EXPECT_EQ(result, nullptr);
  EXPECT_EQ(ctx.getAdjustments().get(a->getId()), nullptr);
  EXPECT_EQ(ctx.getAdjustments().get(b->getId()), nullptr);
}

} // namespace
