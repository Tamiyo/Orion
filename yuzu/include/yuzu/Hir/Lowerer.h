#ifndef YUZU_HIR_LOWERER_H
#define YUZU_HIR_LOWERER_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticBuilder.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirBuilder.h"

#include <string>

namespace yuzu::hir {

/// Lowers an `ast::Root` into an HIR tree allocated in the caller-supplied
/// `HirBuilder`'s arena. Recoverable shape errors (e.g. a binary
/// expression missing an operand) are emitted as diagnostics and replaced
/// with placeholder HIR nodes so passes downstream can still run.
class Lowerer {
public:
  Lowerer(HirBuilder &builder, diagnostics::DiagnosticsEngine &diagnostics,
          diagnostics::SourceId source)
      : builder(builder), diagnostics(diagnostics), source(source) {}

  const Root *lower(const ast::Root &root);
  const Stmt *lowerStmt(const ast::Stmt &stmt);
  const Expr *lowerExpr(const ast::Expr &expr);

private:
  const Expr *lowerBinaryExpr(const ast::BinaryExpr &expr);
  const LiteralExpr *lowerLiteralExpr(const ast::LiteralExpr &expr);

  diagnostics::DiagnosticBuilder error(const ast::AstNode &node,
                                       std::string message);

  HirBuilder &builder;
  diagnostics::DiagnosticsEngine &diagnostics;
  diagnostics::SourceId source;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_LOWERER_H
