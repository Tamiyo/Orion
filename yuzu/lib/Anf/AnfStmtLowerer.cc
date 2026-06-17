#include "yuzu/Anf/AnfLowerer.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/Ops/Op.h"
#include "yuzu/Util/ErrorHandling.h"

#include <vector>

namespace yuzu::anf {
const Stmt *AnfLowerer::lowerStmt(const hir::Stmt *stmt) {
  switch (stmt->getStmtKind()) {
  case hir::StmtKind::ExprStmt:
    return lowerExprStmt(hir::ExprStmt::cast(stmt));
  case hir::StmtKind::StructStmt:
    return lowerStructStmt(hir::StructStmt::cast(stmt));
  case hir::StmtKind::TableStmt:
    return lowerTableStmt(hir::TableStmt::cast(stmt));
  case hir::StmtKind::LetStmt:
    return lowerLetStmt(hir::LetStmt::cast(stmt));
  case hir::StmtKind::AssignStmt:
    return lowerAssignStmt(hir::AssignStmt::cast(stmt));
  case hir::StmtKind::BlockStmt:
    return lowerBlockStmt(hir::BlockStmt::cast(stmt));
  case hir::StmtKind::ReturnStmt:
    return lowerReturnStmt(hir::ReturnStmt::cast(stmt));
  case hir::StmtKind::FuncStmt:
    return lowerFuncStmt(hir::FuncStmt::cast(stmt));
  }
}

const StructFieldDecl *
AnfLowerer::lowerStructFieldDecl(const hir::StructFieldDecl *structFieldDecl) {
  const auto *name = lowerIdent(structFieldDecl->getName());
  const auto *type = ctx.typeOf(structFieldDecl);
  return ctx.build(structFieldDecl, &AnfBuilder::makeStructFieldDecl, name,
                   type);
}
const Stmt *AnfLowerer::lowerExprStmt(const hir::ExprStmt *exprStmt) {
  // Keep the compound as the tail value (a query, a discarded call) — don't
  // atomize it into a temporary.
  const auto *value = lowerExpr(exprStmt->getExpr());
  return ctx.build(exprStmt, &AnfBuilder::makeExprStmt, value);
}

const Stmt *AnfLowerer::lowerStructStmt(const hir::StructStmt *structStmt) {
  const auto *name = lowerIdent(structStmt->getName());

  std::vector<const StructFieldDecl *> fields;
  fields.reserve(structStmt->getFields().size());

  for (const auto *field : structStmt->getFields()) {
    const auto *loweredField = lowerStructFieldDecl(field);
    fields.push_back(loweredField);
  }

  return ctx.build(structStmt, &AnfBuilder::makeStructStmt, name, fields);
}

const Stmt *AnfLowerer::lowerTableStmt(const hir::TableStmt *tableStmt) {
  const auto *ident = lowerIdent(tableStmt->getName());
  const auto *type = ctx.typeOf(tableStmt);
  return ctx.build(tableStmt, &AnfBuilder::makeTableStmt, ident, type);
}

const Stmt *AnfLowerer::lowerLetStmt(const hir::LetStmt *letStmt) {
  const auto *value = lowerExpr(letStmt->getExpr());
  const auto *ident = lowerIdent(letStmt->getName());
  const auto *type = ctx.typeOf(letStmt);
  const auto *binding =
      ctx.build(letStmt, &AnfBuilder::makeBinding, ident, value, type);
  hirToAnfMap[letStmt] =
      ctx.build(letStmt, &AnfBuilder::makeVarAtom, binding, type);

  return ctx.build(letStmt, &AnfBuilder::makeLetStmt, binding);
}

const Stmt *AnfLowerer::lowerAssignStmt(const hir::AssignStmt *assignStmt) {
  const auto *value = lowerExpr(assignStmt->getValue());

  const auto *target = hir::IdentExpr::cast(assignStmt->getTarget());
  if (target == nullptr) {
    // Only identifier targets are lowered today; field/array assignment is
    // future work.
    util::yuzu_unreachable("assignment target must be an identifier");
  }

  const auto *ident = lowerIdent(target->getName());
  const auto *type = ctx.typeOf(assignStmt->getValue());
  const auto *binding =
      ctx.build(assignStmt, &AnfBuilder::makeBinding, ident, value, type);

  // SSA: later reads of this name (the same HIR decl) now see the new binding.
  hirToAnfMap[ctx.getHirContext().resolveIdent(target->getName())] =
      ctx.build(assignStmt, &AnfBuilder::makeVarAtom, binding, type);

  return ctx.build(assignStmt, &AnfBuilder::makeLetStmt, binding);
}

const BlockStmt *AnfLowerer::lowerBlockStmt(const hir::BlockStmt *blockStmt) {
  std::vector<const Stmt *> loweredStmts;
  for (const auto *stmt : blockStmt->getStmts()) {
    const auto *loweredStmt = lowerStmt(stmt);
    // Flush the temporaries this statement produced, then the statement.
    loweredStmts.insert(loweredStmts.end(), intermediateStmts.begin(),
                        intermediateStmts.end());
    intermediateStmts.clear();
    loweredStmts.push_back(loweredStmt);
  }
  return ctx.build(blockStmt, &AnfBuilder::makeBlockStmt, loweredStmts);
}

const Stmt *AnfLowerer::lowerReturnStmt(const hir::ReturnStmt *returnStmt) {
  // A bare `return` (unit) has no value.
  const Atom *value =
      returnStmt->getExpr() ? forceAtom(returnStmt->getExpr()) : nullptr;
  return ctx.build(returnStmt, &AnfBuilder::makeReturnStmt, value);
}

void AnfLowerer::hoistFuncStmt(const hir::FuncStmt *funcStmt) {
  const auto *name = lowerIdent(funcStmt->getName());

  std::vector<const Param *> params;
  params.reserve(funcStmt->getParams().size());
  for (const auto *param : funcStmt->getParams()) {
    params.push_back(lowerParam(param));
  }

  // A placeholder body, filled in by `lowerFuncStmt`. The `FuncRef` points at
  // this shell, so a reference resolves even before the body is lowered.
  const auto *returnType = ctx.typeOf(funcStmt);
  const std::vector<const Stmt *> noStmts;
  const auto *body = ctx.build(funcStmt, &AnfBuilder::makeBlockStmt, noStmts);
  const auto *shell = ctx.build(funcStmt, &AnfBuilder::makeFuncStmt, name,
                                params, body, returnType);
  hirToAnfMap[funcStmt] =
      ctx.build(funcStmt, &AnfBuilder::makeFuncRef, shell, returnType);
}

const Stmt *AnfLowerer::lowerFuncStmt(const hir::FuncStmt *funcStmt) {
  // Top-level functions are hoisted by `lowerRoot`; a nested one (none today)
  // is hoisted on demand. Either way, fill the shell's body now.
  if (hirToAnfMap.lookup(funcStmt) == nullptr) {
    hoistFuncStmt(funcStmt);
  }
  auto *shell = const_cast<FuncStmt *>(
      FuncRef::cast(hirToAnfMap.lookup(funcStmt))->getFunc());
  shell->setBody(lowerBlockStmt(funcStmt->getBody()));
  return shell;
}
} // namespace yuzu::anf