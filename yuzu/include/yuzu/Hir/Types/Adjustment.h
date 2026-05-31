#ifndef YUZU_HIR_TYPES_ADJUSTMENT_H
#define YUZU_HIR_TYPES_ADJUSTMENT_H

#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Util/ErrorHandling.h"

#include <cstdint>
#include <string>

namespace yuzu::hir {

/// How an `Adjustment` reshapes the value of the expression it's bound
/// to.
enum class AdjustmentKind : uint8_t {
  /// Cast the value adjustment's `target` type.
  Cast
};

/// Human-facing name for an `AdjustmentKind`. Rendered into the typed
/// HIR printer's output — e.g. "IntLit : Int64 → cast Float64".
inline std::string asString(AdjustmentKind kind) {
  switch (kind) {
  case AdjustmentKind::Cast:
    return "cast";
  }
  util::yuzu_unreachable();
}

/// A single coercion step applied to an expression's value when it's
/// consumed. The expression's own type comes from its `getType()`;
/// the adjustment records the type the *consumer* expects and the
/// operation needed to bridge the gap.
struct Adjustment {
  AdjustmentKind kind;
  /// Type after the adjustment is applied. Always an interned pointer
  /// from `TypeFactory`, so equality is `==`.
  const Type *target;
};
} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_ADJUSTMENT_H
