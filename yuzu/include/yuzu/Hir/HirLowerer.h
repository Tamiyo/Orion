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
  const Stmt *lowerAssignStmt(ast::AssignStmt stmt);
  const LetStmt *lowerLetStmt(ast::LetStmt stmt);
  const FuncStmt *lowerFuncStmt(ast::FuncStmt stmt);
  const Stmt *lowerStructStmt(ast::StructStmt stmt);
  const StructFieldDecl *lowerStructFieldDecl(ast::StructFieldDecl field);
  const Stmt *lowerTableStmt(ast::TableStmt stmt);
  const BlockStmt *lowerBlockStmt(ast::BlockStmt stmt);
  const ReturnStmt *lowerReturnStmt(ast::ReturnStmt stmt);
  const Param *lowerParam(ast::Param param);
  const TypeBound *lowerTypeBound(ast::TypeBound typeBound);
  const TraitRef *lowerTraitRef(ast::TraitRef traitRef);

  const TypeAnnotation *lowerTypeAnnotation(ast::TypeAnnotation type);
  const NamedTypeAnnotation *
  lowerNamedTypeAnnotation(ast::NamedTypeAnnotation type);
  const FuncTypeAnnotation *
  lowerFuncTypeAnnotation(ast::FuncTypeAnnotation type);

  const Expr *lowerBinaryExpr(ast::BinaryExpr expr);
  const Expr *lowerUnaryExpr(ast::UnaryExpr expr);
  const Expr *lowerParenExpr(ast::ParenExpr expr);
  const Expr *lowerCallExpr(ast::CallExpr expr);
  const IdentExpr *lowerIdentExpr(ast::IdentExpr expr);
  const Expr *lowerFromRel(ast::FromExpr expr);
  const Expr *lowerSelectRel(ast::SelectExpr expr);
  const Expr *lowerWhereRel(ast::WhereExpr expr);
  const Expr *lowerDistinctRel(ast::DistinctExpr expr);
  const Expr *lowerDropRel(ast::DropExpr expr);
  const SelectItem *lowerSelectItem(ast::SelectItem item);
  const Expr *lowerStructExpr(ast::StructExpr expr);
  const StructFieldInit *lowerStructFieldInit(ast::StructFieldInit field);
  const Expr *lowerFieldAccessExpr(ast::FieldAccessExpr expr);

  const Literal *lowerLiteralExpr(ast::Literal expr);
  const BoolLit *lowerBoolLit(ast::BoolLit expr);
  const IntLit *lowerIntLit(ast::IntLit expr);
  const FloatLit *lowerFloatLit(ast::FloatLit expr);
  const StringLit *lowerStringLit(ast::StringLit expr);

  HirContext &ctx;
  diagnostics::DiagnosticsEngine &diagnostics;
  diagnostics::SourceId source;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_LOWERER_H
