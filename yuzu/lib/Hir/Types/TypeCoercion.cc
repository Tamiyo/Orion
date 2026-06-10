#include "yuzu/Hir/Types/TypeCoercion.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Adjustment.h"
#include "yuzu/Types/Type.h"

#include <optional>

namespace yuzu::hir {
using namespace yuzu::types;
namespace {
/// Casts an expression from one type to another, recording the adjustment
/// for later application.
const Type *cast(const Expr *from, const Expr *to, HirContext &ctx) {
  const auto adjustment = Adjustment{.kind = AdjustmentKind::Cast,
                                     .target = ctx.getTypeContext().typeOf(to)};
  ctx.getAdjustments().bind(from->getId(), adjustment);
  return ctx.getTypeContext().typeOf(to);
}

/// Rank in the numeric promotion hierarchy. Higher rank = wider type.
/// Same-width signed/unsigned share a rank — `coerceIntegers` handles
/// the signedness check separately so the rank table stays simple.
std::optional<int> numericRank(const Type *type) {
  switch (type->getKind()) {
  case TypeKind::Int8:
  case TypeKind::UInt8:
    return 1;
  case TypeKind::Int16:
  case TypeKind::UInt16:
    return 2;
  case TypeKind::Int32:
  case TypeKind::UInt32:
    return 3;
  case TypeKind::Int64:
  case TypeKind::UInt64:
    return 4;
  case TypeKind::Float32:
    return 5;
  case TypeKind::Float64:
    return 6;
  default:
    return std::nullopt;
  }
}

/// Promote two operands when at least one is a float. The result lives
/// in the float family — higher-ranked operand wins, so an integer side
/// widens into the float. Precision loss for large integers (e.g.
/// `Int64 → Float64`) is the caller's risk.
const Type *coerceFloats(const Expr *a, const Expr *b, HirContext &ctx) {
  const auto ra = numericRank(ctx.getTypeContext().typeOf(a));
  const auto rb = numericRank(ctx.getTypeContext().typeOf(b));

  return (*ra < *rb) ? cast(a, b, ctx) : cast(b, a, ctx);
}

/// Promote two integer operands. Same signedness widens to the
/// higher-ranked side; mixed signed/unsigned refuses implicit coercion
/// to avoid silent value corruption (`Int8(-1) → UInt8` becomes 255,
/// etc.). The caller writes an explicit cast.
const Type *coerceIntegers(const Expr *a, const Expr *b, HirContext &ctx) {
  const auto *aType = ctx.getTypeContext().typeOf(a);
  const auto *bType = ctx.getTypeContext().typeOf(b);
  if (aType->isUnsigned() != bType->isUnsigned()) {
    return nullptr;
  }
  const auto ra = numericRank(aType);
  const auto rb = numericRank(bType);

  return (*ra < *rb) ? cast(a, b, ctx) : cast(b, a, ctx);
}
} // namespace

const Type *coerceTypes(const Expr *a, const Expr *b, HirContext &ctx) {
  auto &types = ctx.getTypeContext();
  const auto *aType = types.typeOf(a);
  const auto *bType = types.typeOf(b);

  // Unify first: identical types, or an untyped literal adopting its
  // partner (and `5 + 5` stays one open hole the caller can still pin).
  if (types.unifyTypes(aType, bType)) {
    return aType;
  }

  // Refused — pin any holes to their defaults, then widen numerically.
  aType = types.resolveType(aType);
  bType = types.resolveType(bType);
  types.bind(a, aType);
  types.bind(b, bType);

  if (!aType->isNumeric() || !bType->isNumeric()) {
    return nullptr;
  }
  if (aType->isFloat() || bType->isFloat()) {
    return coerceFloats(a, b, ctx);
  }
  return coerceIntegers(a, b, ctx);
}

bool coercesTo(const Expr *value, const Type *target, HirContext &ctx) {
  const auto *source = ctx.getTypeContext().typeOf(value);
  if (source == target) {
    return true;
  }
  if (!source->isNumeric() || !target->isNumeric()) {
    return false;
  }

  // Only widening is implicit: rank must not shrink, and signedness must
  // match (any integer also widens into a float). Record the cast.
  if (*numericRank(source) > *numericRank(target)) {
    return false;
  }
  if (!target->isFloat() && source->isUnsigned() != target->isUnsigned()) {
    return false;
  }
  ctx.getAdjustments().bind(
      value->getId(),
      Adjustment{.kind = AdjustmentKind::Cast, .target = target});
  return true;
}

} // namespace yuzu::hir
