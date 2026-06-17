#ifndef YUZU_HIR_OPS_OPRESOLVE_H
#define YUZU_HIR_OPS_OPRESOLVE_H

#include "yuzu/Ops/BuiltinOp.h"

#include <llvm/ADT/ArrayRef.h>

namespace yuzu::types {
class Type;
}

namespace yuzu::hir {
class Expr;
class HirContext;

/// Resolve the result type of `op` applied to `args` (operands already typed),
/// emitting a diagnostic and returning the error type on a mismatch. Called
/// during type inference so the constructed `CallExpr` carries its type from
/// the start.
const types::Type *resolve(BuiltinOp op, llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx);

} // namespace yuzu::hir

#endif // YUZU_HIR_OPS_OPRESOLVE_H
