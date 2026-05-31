#ifndef YUZU_HIR_TYPES_TYPECOERCION_H
#define YUZU_HIR_TYPES_TYPECOERCION_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Type.h"

namespace yuzu::hir {
const Type *coerceTypes(const Expr *a, const Expr *b, HirContext &ctx);

/// True if `value` is assignable to `target` — same type, or a lossless
/// numeric widening (records a cast adjustment on `value`).
bool coercesTo(const Expr *value, const Type *target, HirContext &ctx);
} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPECOERCION_H
