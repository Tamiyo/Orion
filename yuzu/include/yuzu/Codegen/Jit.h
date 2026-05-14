#ifndef YUZU_CODEGEN_JIT_H
#define YUZU_CODEGEN_JIT_H

#include "mlir/IR/BuiltinOps.h"
#include "mlir/IR/MLIRContext.h"

#include <cstdint>
#include <optional>

namespace yuzu::codegen {

/// Lower `module` (which must contain `func.func @yuzu_main() -> i64`)
/// from arith/func to the LLVM dialect, translate it to LLVM IR, JIT it,
/// and call `yuzu_main`. Returns the function's result, or `std::nullopt`
/// if any stage fails.
std::optional<int64_t> jitExecute(mlir::MLIRContext &ctx,
                                  mlir::ModuleOp module);

} // namespace yuzu::codegen

#endif // YUZU_CODEGEN_JIT_H
