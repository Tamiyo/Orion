#include "yuzu/Hir/Types/TypeChecker.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Util/ErrorHandling.h"

namespace yuzu::hir {
void TypeChecker::check(const Root *root) {
  for (const auto *stmt : root->getStmts()) {
    checkStmt(stmt);
  }
}

void TypeChecker::checkStmt(const Stmt *s) {
  switch (s->getStmtKind()) {
  case StmtKind::LetStmt: {
    checkLetStmt(LetStmt::cast(s));
    return;
  }
  case StmtKind::ExprStmt: {
    checkExprStmt(ExprStmt::cast(s));
    return;
  }
  }
}

// TODO - Symbol table resolution.
void TypeChecker::checkLetStmt(const LetStmt *s) { checkExpr(s->getExpr()); }

void TypeChecker::checkExprStmt(const ExprStmt *s) { checkExpr(s->getExpr()); }

const Type *TypeChecker::checkExpr(const Expr *e) {
  switch (e->getExprKind()) {
  case ExprKind::CallExpr:
    return checkCallExpr(CallExpr::cast(e));
  case ExprKind::Literal:
    return checkLiteral(Literal::cast(e));
  default:
    util::yuzu_unreachable("unexpected variant in TypeChecker::checkExpr");
  }
}

const Type *TypeChecker::checkCallExpr(const CallExpr *e) {
  // The lowerer already resolved the call's type; this walk just
  // descends into operands so any future per-operand validation can
  // run uniformly.
  for (const Expr *arg : e->getArgs()) {
    checkExpr(arg);
  }
  return e->getType();
}

const Type *TypeChecker::checkLiteral(const Literal *l) {
  switch (l->getLiteralKind()) {
  case LiteralKind::BoolLit:
  case LiteralKind::IntLit:
  case LiteralKind::FloatLit:
  case LiteralKind::StringLit:
    return l->getType();
  default:
    util::yuzu_unreachable("unexpected variant in TypeChecker::checkLiteral");
  }
}
} // namespace yuzu::hir
