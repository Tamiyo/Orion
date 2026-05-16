#include "yuzu/Hir/Types/TypeCoercion.h"

#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Type.h"

#include <optional>

namespace yuzu::hir {
namespace {

/// Rank in the numeric promotion hierarchy. Higher rank = wider type.
/// Same-width signed/unsigned share a rank — `coerceIntegers` handles
/// the signedness check separately so the rank table stays simple.
std::optional<int> numericRank(TypeKind k) {
  switch (k) {
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
const Type *coerceFloats(const Type *a, const Type *b) {
  const auto ra = numericRank(a->getKind());
  const auto rb = numericRank(b->getKind());
  return *ra < *rb ? b : a;
}

/// Promote two integer operands. Same signedness widens to the
/// higher-ranked side; mixed signed/unsigned refuses implicit coercion
/// to avoid silent value corruption (`Int8(-1) → UInt8` becomes 255,
/// etc.). The caller writes an explicit cast.
const Type *coerceIntegers(const Type *a, const Type *b) {
  if (a->isUnsigned() != b->isUnsigned()) {
    return nullptr;
  }
  const auto ra = numericRank(a->getKind());
  const auto rb = numericRank(b->getKind());
  return *ra < *rb ? b : a;
}

} // namespace

const Type *coerceTypes(const Type *a, const Type *b, HirContext &) {
  if (a->getKind() == b->getKind()) {
    return a;
  }
  if (!a->isNumeric() || !b->isNumeric()) {
    return nullptr;
  }
  if (a->isFloat() || b->isFloat()) {
    return coerceFloats(a, b);
  }
  return coerceIntegers(a, b);
}

} // namespace yuzu::hir
