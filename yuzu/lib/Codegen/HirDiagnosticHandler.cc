#include "yuzu/Codegen/HirDiagnosticHandler.h"

#include "yuzu/Codegen/HirLocation.h"
#include "yuzu/Diagnostics/DiagnosticBuilder.h"

#include "mlir/IR/Diagnostics.h"

#include <string>
#include <utility>

namespace yuzu::codegen {
namespace {
diagnostics::Span spanFor(const hir::HirSourceMap &sourceMap,
                         diagnostics::SourceId source, mlir::Location loc) {
  if (const auto id = hirIdFromLoc(loc)) {
    if (const auto astNode = sourceMap.get(*id)) {
      const auto range = astNode->getRange();
      return diagnostics::Span{source, range.start, range.end};
    }
  }
  return diagnostics::Span{source, 0, 0};
}

diagnostics::DiagnosticBuilder
beginDiagnostic(diagnostics::DiagnosticsEngine &engine,
                mlir::DiagnosticSeverity severity, diagnostics::Span span,
                std::string message) {
  switch (severity) {
  case mlir::DiagnosticSeverity::Warning:
    return engine.warning(span, std::move(message));
  case mlir::DiagnosticSeverity::Note:
  case mlir::DiagnosticSeverity::Remark:
    return engine.remark(span, std::move(message));
  case mlir::DiagnosticSeverity::Error:
    break;
  }
  return engine.error(span, std::move(message));
}
} // namespace

void installHirDiagnosticHandler(mlir::MLIRContext &ctx,
                                 const hir::HirSourceMap &sourceMap,
                                 diagnostics::SourceId source,
                                 diagnostics::DiagnosticsEngine &engine) {
  ctx.getDiagEngine().registerHandler(
      [&sourceMap, source, &engine](mlir::Diagnostic &diag) {
        const auto span = spanFor(sourceMap, source, diag.getLocation());
        beginDiagnostic(engine, diag.getSeverity(), span, diag.str()).emit();
        return mlir::success();
      });
}

} // namespace yuzu::codegen
