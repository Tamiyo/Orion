#ifndef YUZU_ANF_ANFLOWERER_H
#define YUZU_ANF_ANFLOWERER_H

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Anf/Ops/Op.h"
#include "yuzu/Diagnostics/DiagnosticBuilder.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/Op.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>

#include <cstdint>
#include <string>
#include <vector>

namespace yuzu::anf {
class AnfLowerer final {
public:
  AnfLowerer(AnfContext &ctx, hir::HirContext &hir,
             diagnostics::DiagnosticsEngine &diagnostics,
             diagnostics::SourceId source)
      : ctx(ctx), hirCtx(hir), diagnostics(diagnostics), source(source) {}

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

  const Expr *lowerExpr(const hir::Expr *expr);
  const Expr *lowerIdentExpr(const hir::IdentExpr *identExpr);
  const Expr *lowerCallExpr(const hir::CallExpr *callExpr);
  const Expr *lowerFuncCallExpr(const hir::FuncCallExpr *funcCallExpr);
  const Expr *lowerFieldAccessExpr(const hir::FieldAccessExpr *fieldAccessExpr);
  const Expr *lowerStructExpr(const hir::StructExpr *structExpr);
  const Atom *resolveCallee(const hir::Expr *callee);

  const Rel *lowerRel(const hir::Rel *rel);
  const Rel *lowerFromRel(const hir::FromRel *fromRel);
  const Rel *lowerSelectRel(const hir::SelectRel *selectRel);
  const SelectItem *lowerSelectItem(const hir::SelectItem *selectItem);

  const Constant *lowerLiteral(const hir::Literal *literal);
  const BoolConst *lowerBoolLit(const hir::BoolLit *boolLit);
  const IntConst *lowerIntLit(const hir::IntLit *intLit);
  const FloatConst *lowerFloatLit(const hir::FloatLit *floatLit);
  const StringConst *lowerStringLit(const hir::StringLit *stringLit);

  const StructFieldDecl *
  lowerStructFieldDecl(const hir::StructFieldDecl *structFieldDecl);
  const Ident *lowerIdent(const hir::Ident *ident);
  const Param *lowerParam(const hir::Param *param);
  const Op *lowerOp(const hir::Op *op);

private:
  /// Lowers an HIR expression into an Atom, forcibely binding complex
  // expressions to t
  const Atom *forceAtom(const hir::Expr *expr) {
    const auto *lowered = lowerExpr(expr);
    if (const auto *atom = Atom::cast(lowered))
      return atom;

    const auto *type = hirCtx.getTypeContext().typeOf(expr);
    const auto *binding =
        ctx.build(expr, &AnfBuilder::makeBinding, makeTemp(), lowered, type);
    intermediateStmts.push_back(
        ctx.build(expr, &AnfBuilder::makeLetStmt, binding));
    return ctx.build(expr, &AnfBuilder::makeVarAtom, binding, type);
  }

  const Ident *makeTemp() {
    std::u32string name = U"%t";
    for (char c : std::to_string(tempCounter++)) {
      name.push_back(static_cast<char32_t>(c));
    }
    return ctx.getBuilder().makeIdent(hirCtx.getStringInterner().intern(name));
  }

  AnfContext &ctx;
  hir::HirContext &hirCtx;
  diagnostics::DiagnosticsEngine &diagnostics;
  diagnostics::SourceId source;

  /// HIR declaration -> its ANF `Binding`. Written as each `let`/param lowers,
  /// overwritten on reassignment (SSA); read to resolve `VarAtom`s. Keyed by
  /// the decl node, so shadowing and mutation need no scope stack.
  llvm::DenseMap<const hir::HirNode *, const Binding *> hirToAnfBindingMap;

  /// HIR `FuncStmt` decl -> its lowered ANF `FuncStmt`. Written as each
  /// function lowers; read by `resolveCallee` to emit a `FuncRef` for a direct
  /// call so a later inlining pass can chase the target.
  llvm::DenseMap<const hir::HirNode *, const FuncStmt *> hirToAnfFuncMap;

  /// Temp `LetStmt`s introduced while lowering the current statement;
  /// `lowerBlockStmt` flushes them into the block before that statement.
  std::vector<const Stmt *> intermediateStmts;

  uint32_t tempCounter = 0;
};

} // namespace yuzu::anf

#endif // YUZU_ANF_ANFLOWERER_H
