#include "yuzu/Codegen/HirDiagnosticHandler.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Codegen/HirLocation.h"
#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirSourceMap.h"
#include "yuzu/Syntax/Green/Green.h"

#include "mlir/IR/Diagnostics.h"
#include "mlir/IR/Location.h"
#include "mlir/IR/MLIRContext.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

namespace {

using yuzu::ast::IntLit;
using yuzu::ast::SyntaxKind;
using yuzu::ast::SyntaxNode;
using yuzu::codegen::installHirDiagnosticHandler;
using yuzu::codegen::locFor;
using yuzu::diagnostics::Severity;
using yuzu::hir::HirId;
using yuzu::hir::HirSourceMap;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;

class HirDiagnosticHandlerTest : public ::testing::Test {
protected:
  yuzu::diagnostics::SourceMap sources;
  yuzu::diagnostics::DiagnosticsEngine engine;
  yuzu::diagnostics::SourceId sourceId = sources.add("<test>", U"42");
  HirSourceMap sourceMap;
  mlir::MLIRContext ctx;

  /// Create a `LiteralExpr`-kinded SyntaxNode with a single `Number`
  /// token of `text`, bind it to `id` in the source map, and return the
  /// `OpaqueLoc` referring to that id.
  mlir::Location bindLiteral(HirId id, std::u32string_view text) {
    const GreenToken token(static_cast<uint16_t>(SyntaxKind::IntegerLiteral),
                           std::u32string(text));
    const auto green = GreenNode::create(
        static_cast<uint16_t>(SyntaxKind::IntLit),
        std::vector<GreenElement>{GreenElement(token)});
    const IntLit lit{SyntaxNode::createRoot(green)};
    sourceMap.bind(id, lit);
    return locFor(ctx, id);
  }
};

TEST_F(HirDiagnosticHandlerTest, ErrorRoutesThroughEngineWithAstSpan) {
  const auto loc = bindLiteral(HirId{1}, U"42");
  installHirDiagnosticHandler(ctx, sourceMap, sourceId, engine);

  mlir::emitError(loc) << "boom";

  ASSERT_EQ(engine.getDiagnostics().size(), 1u);
  const auto &d = engine.getDiagnostics()[0];
  EXPECT_EQ(d.severity, Severity::Error);
  EXPECT_EQ(d.message, "boom");
  ASSERT_FALSE(d.labels.empty());
  EXPECT_EQ(d.labels[0].span.source, sourceId);
  EXPECT_EQ(d.labels[0].span.start, 0u);
  EXPECT_EQ(d.labels[0].span.end, 2u);
}

TEST_F(HirDiagnosticHandlerTest, WarningPreservesSeverity) {
  const auto loc = bindLiteral(HirId{2}, U"42");
  installHirDiagnosticHandler(ctx, sourceMap, sourceId, engine);

  mlir::emitWarning(loc) << "careful";

  ASSERT_EQ(engine.getDiagnostics().size(), 1u);
  const auto &d = engine.getDiagnostics()[0];
  EXPECT_EQ(d.severity, Severity::Warning);
  EXPECT_EQ(d.message, "careful");
}

TEST_F(HirDiagnosticHandlerTest, RemarkPreservesSeverity) {
  const auto loc = bindLiteral(HirId{3}, U"42");
  installHirDiagnosticHandler(ctx, sourceMap, sourceId, engine);

  mlir::emitRemark(loc) << "fyi";

  ASSERT_EQ(engine.getDiagnostics().size(), 1u);
  EXPECT_EQ(engine.getDiagnostics()[0].severity, Severity::Remark);
}

TEST_F(HirDiagnosticHandlerTest, UnknownLocationFallsBackToZeroWidthSpan) {
  installHirDiagnosticHandler(ctx, sourceMap, sourceId, engine);

  mlir::emitError(mlir::UnknownLoc::get(&ctx)) << "no origin";

  ASSERT_EQ(engine.getDiagnostics().size(), 1u);
  const auto &d = engine.getDiagnostics()[0];
  ASSERT_FALSE(d.labels.empty());
  EXPECT_EQ(d.labels[0].span.source, sourceId);
  EXPECT_EQ(d.labels[0].span.start, 0u);
  EXPECT_EQ(d.labels[0].span.end, 0u);
}

TEST_F(HirDiagnosticHandlerTest, UnboundHirIdFallsBackToZeroWidthSpan) {
  // HirId is encoded but no AST node was bound for it — the handler
  // emits at a zero-width span rather than crashing.
  installHirDiagnosticHandler(ctx, sourceMap, sourceId, engine);

  mlir::emitError(locFor(ctx, HirId{99})) << "orphan";

  ASSERT_EQ(engine.getDiagnostics().size(), 1u);
  const auto &d = engine.getDiagnostics()[0];
  EXPECT_EQ(d.labels[0].span.start, 0u);
  EXPECT_EQ(d.labels[0].span.end, 0u);
}

} // namespace
