#ifndef YUZU_ANF_ANFLOWERER_H
#define YUZU_ANF_ANFLOWERER_H

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Diagnostics/DiagnosticBuilder.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>

#include <vector>

namespace yuzu::anf {
class AnfLowerer final {
public:
  AnfLowerer(AnfContext &ctx, diagnostics::DiagnosticsEngine &diagnostics,
             diagnostics::SourceId source)
      : ctx(ctx), diagnostics(diagnostics), source(source) {}

  AnfLowerer(const AnfLowerer &) = default;
  AnfLowerer(AnfLowerer &&) = default;
  AnfLowerer &operator=(const AnfLowerer &) = delete;
  AnfLowerer &operator=(AnfLowerer &&) = delete;

  const Root *lowerRoot(const hir::Root *root);

  const Stmt *lowerStmt(const hir::Stmt *stmt);
  const Stmt *lowerExprStmt(const hir::ExprStmt *exprStmt);
  const Stmt *lowerStructStmt(const hir::StructStmt *structStmt);
  const Stmt *lowerTableStmt(const hir::TableStmt *tableStmt);
  const Stmt *lowerLetStmt(const hir::LetStmt *letStmt);
  const Stmt *lowerAssignStmt(const hir::AssignStmt *assignStmt);
  const BlockStmt *lowerBlockStmt(const hir::BlockStmt *blockStmt);
  const Stmt *lowerReturnStmt(const hir::ReturnStmt *returnStmt);
  const Stmt *lowerFuncStmt(const hir::FuncStmt *funcStmt);

  /// Pre-create a function's shell (signature + `FuncRef`) before any body is
  /// lowered, so a call to a function declared later still resolves.
  void hoistFuncStmt(const hir::FuncStmt *funcStmt);

  const Expr *lowerExpr(const hir::Expr *expr);
  const Expr *lowerIdentExpr(const hir::IdentExpr *identExpr);
  const Expr *lowerCallExpr(const hir::CallExpr *callExpr);
  const Expr *lowerFuncCallExpr(const hir::FuncCallExpr *funcCallExpr);
  const Expr *lowerFieldAccessExpr(const hir::FieldAccessExpr *fieldAccessExpr);
  const Expr *lowerStructExpr(const hir::StructExpr *structExpr);

  const Rel *lowerRel(const hir::Rel *rel);
  const Rel *lowerFromRel(const hir::FromRel *fromRel);
  const Rel *lowerSelectRel(const hir::SelectRel *selectRel);
  const Rel *lowerWhereRel(const hir::WhereRel *whereRel);
  const Rel *lowerDistinctRel(const hir::DistinctRel *distinctRel);
  const SelectItem *lowerSelectItem(const hir::SelectItem *selectItem);

  /// Lower `expr` into a `Thunk` — a per-row block whose temporaries are
  /// captured into a fresh buffer and whose tail yields the value. Used for
  /// `select` column bodies and `where` predicates. `origin` is the HIR node
  /// the synthesized nodes attribute to for diagnostics.
  const Thunk *lowerThunk(const hir::Expr *expr, const hir::HirNode *origin);

  /// Bind a `select`'s `as`-aliased columns into `hirToAnf` so a later stage's
  /// reference to one lowers to a `FieldAtom` selecting it from this relation.
  void bindColumns(const hir::SelectRel *selectRel);

  const Constant *lowerLiteral(const hir::Literal *literal);
  const BoolConst *lowerBoolLit(const hir::BoolLit *boolLit);
  const IntConst *lowerIntLit(const hir::IntLit *intLit);
  const FloatConst *lowerFloatLit(const hir::FloatLit *floatLit);
  const StringConst *lowerStringLit(const hir::StringLit *stringLit);

  const StructFieldDecl *
  lowerStructFieldDecl(const hir::StructFieldDecl *structFieldDecl);
  const Ident *lowerIdent(const hir::Ident *ident);
  const Param *lowerParam(const hir::Param *param);

private:
  /// Lowers an HIR expression into an Atom, forcibely binding complex
  // expressions to t
  const Atom *forceAtom(const hir::Expr *expr) {
    const auto *lowered = lowerExpr(expr);
    if (const auto *atom = Atom::cast(lowered))
      return atom;

    const auto *type = ctx.getHirContext().getTypeContext().typeOf(expr);
    const auto *binding = ctx.build(expr, &AnfBuilder::makeBinding,
                                    ctx.makeTemp(), lowered, type);
    intermediateStmts.push_back(
        ctx.build(expr, &AnfBuilder::makeLetStmt, binding));
    return ctx.build(expr, &AnfBuilder::makeVarAtom, binding, type);
  }

  AnfContext &ctx;
  diagnostics::DiagnosticsEngine &diagnostics;
  diagnostics::SourceId source;

  /// Maps each HIR declaration to the ANF atom a reference to it lowers to.
  llvm::DenseMap<const hir::HirNode *, const Atom *> hirToAnfMap;

  /// Tracks temporary statements introduced by lowering.
  std::vector<const Stmt *> intermediateStmts;
};

} // namespace yuzu::anf

#endif // YUZU_ANF_ANFLOWERER_H
