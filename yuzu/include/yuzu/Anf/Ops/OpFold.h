#ifndef YUZU_ANF_OPS_OPFOLD_H
#define YUZU_ANF_OPS_OPFOLD_H

#include "yuzu/Ops/BuiltinOp.h"

#include <llvm/ADT/ArrayRef.h>

namespace yuzu::types {
class Type;
}

namespace yuzu::anf {
class Atom;
class Constant;
class AnfContext;

/// Constant-fold `op` applied to `args` that have already been resolved to
/// their defining values. Returns the result constant, or null when it can't
/// fold — a non-constant operand, an unsupported combination, or a case that
/// would trap at runtime (e.g. integer division by zero). `type` is the result
/// type carried on the `CallExpr`.
const Constant *fold(BuiltinOp op, llvm::ArrayRef<const Atom *> args,
                     AnfContext &ctx, const types::Type *type);

} // namespace yuzu::anf

#endif // YUZU_ANF_OPS_OPFOLD_H
