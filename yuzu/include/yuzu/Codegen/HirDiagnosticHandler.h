#ifndef YUZU_CODEGEN_HIR_DIAGNOSTIC_HANDLER_H
#define YUZU_CODEGEN_HIR_DIAGNOSTIC_HANDLER_H

#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/HirSourceMap.h"

#include "mlir/IR/MLIRContext.h"

namespace yuzu::codegen {

/// Register an `mlir::DiagnosticEngine` handler that re-emits each MLIR
/// diagnostic through `engine`, resolving the source span via the HIR
/// `OpaqueLoc` payload and `sourceMap`. Diagnostics fired on ops without
/// a recoverable HIR origin (passes that inserted ops, lost provenance,
/// etc.) land on a zero-width span at the start of `source`.
void installHirDiagnosticHandler(mlir::MLIRContext &ctx,
                                 const hir::HirSourceMap &sourceMap,
                                 diagnostics::SourceId source,
                                 diagnostics::DiagnosticsEngine &engine);

} // namespace yuzu::codegen

#endif // YUZU_CODEGEN_HIR_DIAGNOSTIC_HANDLER_H
