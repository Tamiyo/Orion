#ifndef YUZU_HIR_HIRLOWERER_H
#define YUZU_HIR_HIRLOWERER_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticBuilder.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirBuilder.h"
#include "yuzu/Hir/HirContext.h"

#include <string>

namespace yuzu::hir {

/// Lowers an `ast::Root` into an HIR tree allocated in the caller-supplied
/// `HirBuilder`'s arena. Recoverable shape errors (e.g. a binary
/// expression missing an operand) are emitted as diagnostics and replaced
/// with placeholder HIR nodes so passes downstream can still run. Every
/// HIR node built during lowering is bound back to its AST origin in
/// `sourceMap`.
class HirLowerer {
public:
  explicit HirLowerer(HirContext &ctx,
                      diagnostics::DiagnosticsEngine &diagnostics,
                      diagnostics::SourceId source)
      : ctx(ctx), diagnostics(diagnostics), source(source) {}

  const Root *lower(ast::Root root);
  const Stmt *lowerStmt(ast::Stmt stmt);
  const Expr *lowerExpr(ast::Expr expr);

  /// Start an error diagnostic at the AST node's source range.
  diagnostics::DiagnosticBuilder error(ast::AstNode node, std::string message);
  /// Start an error diagnostic at the source range of the AST origin
  /// bound to `node->getId()` in the source map.
  diagnostics::DiagnosticBuilder error(const HirNode *node,
                                       std::string message);

private:
  const Ident *lowerIdent(ast::Ident ident);

  const Stmt *lowerExprStmt(ast::ExprStmt stmt);
  const LetStmt *lowerLetStmt(ast::LetStmt stmt);

  const Expr *lowerBinaryExpr(ast::BinaryExpr expr);
  const Expr *lowerParenExpr(ast::ParenExpr expr);
  const IdentExpr *lowerIdentExpr(ast::IdentExpr expr);

  const Literal *lowerLiteralExpr(ast::Literal expr);
  const BoolLit *lowerBoolLit(ast::BoolLit expr);
  const IntLit *lowerIntLit(ast::IntLit expr);
  const FloatLit *lowerFloatLit(ast::FloatLit expr);
  const StringLit *lowerStringLit(ast::StringLit expr);

  const Type *lowerType(ast::TypeExpr type);
  const Type *lowerNamedType(ast::NamedType type);

  HirContext &ctx;
  diagnostics::DiagnosticsEngine &diagnostics;
  diagnostics::SourceId source;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_LOWERER_H
