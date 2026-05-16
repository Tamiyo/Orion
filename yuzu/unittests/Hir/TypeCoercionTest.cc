#include "yuzu/Hir/Types/TypeCoercion.h"

#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeInterner.h"

#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace {

using namespace yuzu::hir;
using yuzu::diagnostics::DiagnosticsEngine;
using yuzu::diagnostics::SourceId;

const Type *typeFor(const TypeInterner &i, TypeKind k) {
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

struct CoercionCase {
  std::string name;
  TypeKind a;
  TypeKind b;
  std::optional<TypeKind> expected;
};

class TypeCoercionTest : public ::testing::TestWithParam<CoercionCase> {
protected:
  DiagnosticsEngine diagnostics;
  HirContext ctx{diagnostics, SourceId{}};
  TypeInterner interner;
};

TEST_P(TypeCoercionTest, Coerces) {
  const auto &c = GetParam();
  const auto *result =
      coerceTypes(typeFor(interner, c.a), typeFor(interner, c.b), ctx);

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

// `coerceTypes` should yield the same result kind regardless of operand
// order — symmetry is part of the contract since the function is meant
// for commutative arithmetic.
TEST_F(TypeCoercionTest, IsSymmetric) {
  const TypeKind pairs[][2] = {
      {TypeKind::Int8, TypeKind::Int64},    {TypeKind::UInt8, TypeKind::UInt32},
      {TypeKind::Int32, TypeKind::Float32}, {TypeKind::Int32, TypeKind::UInt32},
      {TypeKind::Str, TypeKind::Int64},
  };
  for (const auto &pair : pairs) {
    const auto *ab = coerceTypes(typeFor(interner, pair[0]),
                                 typeFor(interner, pair[1]), ctx);
    const auto *ba = coerceTypes(typeFor(interner, pair[1]),
                                 typeFor(interner, pair[0]), ctx);
    if (ab == nullptr || ba == nullptr) {
      EXPECT_EQ(ab, ba) << asString(pair[0]) << " / " << asString(pair[1]);
      continue;
    }
    EXPECT_EQ(ab->getKind(), ba->getKind())
        << asString(pair[0]) << " / " << asString(pair[1]);
  }
}

} // namespace
