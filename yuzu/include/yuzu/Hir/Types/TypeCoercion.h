#ifndef YUZU_HIR_TYPES_TYPECOERCION_H
#define YUZU_HIR_TYPES_TYPECOERCION_H

#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Type.h"

namespace yuzu::hir {
const Type *coerceTypes(const Type *a, const Type *b, HirContext &ctx);
} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPECOERCION_H
