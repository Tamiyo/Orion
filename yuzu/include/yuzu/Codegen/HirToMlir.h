#ifndef YUZU_CODEGEN_HIR_TO_MLIR_H
#define YUZU_CODEGEN_HIR_TO_MLIR_H

#include "yuzu/Hir/Hir.h"

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/OwningOpRef.h"

namespace yuzu::codegen {

/// Translate an HIR `Root` into an `mlir::ModuleOp` containing a
/// `func.func @yuzu_main() -> i64` that, when invoked, returns the value
/// of the last expression. Built using the `arith` and `func` dialects;
/// no custom dialect.
mlir::OwningOpRef<mlir::ModuleOp> lowerHirToMlir(mlir::MLIRContext &ctx,
                                                 const hir::Root *root);

} // namespace yuzu::codegen

#endif // YUZU_CODEGEN_HIR_TO_MLIR_H
