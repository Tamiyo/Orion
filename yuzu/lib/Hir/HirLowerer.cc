#include "yuzu/Hir/HirLowerer.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Hir/Ops/BuiltinOps.h"
#include "yuzu/Hir/Ops/Op.h"
#include "yuzu/Util/ErrorHandling.h"

#include <vector>

namespace yuzu::hir {
namespace {
const Op *toHir(ast::BinOp op) {
  switch (op) {
  case ast::BinOp::Add:
    return AddOp::get();
  case ast::BinOp::Sub:
    return SubOp::get();
  case ast::BinOp::Mul:
    return MulOp::get();
  case ast::BinOp::Div:
    return DivOp::get();
  }
  util::yuzu_unreachable();
}
} // namespace

const Root *HirLowerer::lower(ast::Root root) {
  std::vector<const Stmt *> stmts;
  for (const ast::Stmt &stmt : root.getStmts()) {
    if (const Stmt *lowered = lowerStmt(stmt)) {
      stmts.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeRoot(stmts);
  ctx.getSourceTable().bind(hir->getId(), root);

  return hir;
}

const Stmt *HirLowerer::lowerStmt(ast::Stmt stmt) {
  switch (stmt.getKind()) {
  case ast::SyntaxKind::ExprStmt:
    return lowerExprStmt(*ast::ExprStmt::cast(stmt));
  default:
    util::yuzu_unreachable();
  }
}

const Stmt *HirLowerer::lowerExprStmt(ast::ExprStmt stmt) {
  const auto expr = stmt.getExpr();
  if (!expr) {
    error(stmt, "incomplete statement").emit();
    return nullptr;
  }
  const Expr *loweredExpr = lowerExpr(*expr);
  if (!loweredExpr) {
    return nullptr;
  }
  const auto *hir = ctx.getBuilder().makeExprStmt(loweredExpr);
  ctx.getSourceTable().bind(hir->getId(), stmt);
  return hir;
}

const Expr *HirLowerer::lowerExpr(ast::Expr expr) {
  switch (expr.getKind()) {
  case ast::SyntaxKind::BinaryExpr:
    return lowerBinaryExpr(*ast::BinaryExpr::cast(expr));
  case ast::SyntaxKind::BoolLit:
  case ast::SyntaxKind::IntLit:
  case ast::SyntaxKind::FloatLit:
  case ast::SyntaxKind::StringLit:
    return lowerLiteralExpr(*ast::Literal::cast(expr));
  default:
    util::yuzu_unreachable();
  }
}

const Expr *HirLowerer::lowerBinaryExpr(ast::BinaryExpr expr) {
  const auto lhs = expr.getLhs();
  const auto op = expr.getOp();
  const auto rhs = expr.getRhs();
  if (!lhs) {
    error(expr, "binary expression is missing its left operand").emit();
    return nullptr;
  }
  if (!op) {
    error(expr, "binary expression is missing its operator").emit();
    return nullptr;
  }
  if (!rhs) {
    error(expr, "binary expression is missing its right operand").emit();
    return nullptr;
  }
  const Expr *loweredLhs = lowerExpr(*lhs);
  const Expr *loweredRhs = lowerExpr(*rhs);
  if (!loweredLhs || !loweredRhs) {
    return nullptr;
  }

  // Resolve the call's result type from the operand types so the
  // CallExpr is born with the right type — no follow-up patching needed.
  const std::array<const Expr *, 2> args = {loweredLhs, loweredRhs};
  const auto *loweredOp = toHir(*op);
  const auto *type = loweredOp->resolve(args, ctx);

  const auto *hir = ctx.getBuilder().makeCallExpr(loweredOp, args, type);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const Literal *HirLowerer::lowerLiteralExpr(ast::Literal expr) {
  switch (expr.getKind()) {
  case ast::SyntaxKind::BoolLit:
    return lowerBoolLit(*ast::BoolLit::cast(expr));
  case ast::SyntaxKind::IntLit:
    return lowerIntLit(*ast::IntLit::cast(expr));
  case ast::SyntaxKind::FloatLit:
    return lowerFloatLit(*ast::FloatLit::cast(expr));
  case ast::SyntaxKind::StringLit:
    return lowerStringLit(*ast::StringLit::cast(expr));
  default:
    util::yuzu_unreachable();
  }
}

const BoolLit *HirLowerer::lowerBoolLit(ast::BoolLit expr) {
  const auto value = expr.getValue();
  if (!value) {
    error(expr, "bool literal is missing its value").emit();
    return nullptr;
  }
  const auto *type = ctx.getTypeInterner().getBool();
  const auto *hir = ctx.getBuilder().makeBoolLit(*value, type);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const IntLit *HirLowerer::lowerIntLit(ast::IntLit expr) {
  const auto value = expr.getValue();
  if (!value) {
    error(expr, "integer literal is missing its value").emit();
    return nullptr;
  }
  const auto *type = ctx.getTypeInterner().getInt64();
  const auto *hir = ctx.getBuilder().makeIntLit(*value, type);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const FloatLit *HirLowerer::lowerFloatLit(ast::FloatLit expr) {
  const auto value = expr.getValue();
  if (!value) {
    error(expr, "float literal is missing its value").emit();
    return nullptr;
  }
  const auto *type = ctx.getTypeInterner().getFloat64();
  const auto *hir = ctx.getBuilder().makeFloatLit(*value, type);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const StringLit *HirLowerer::lowerStringLit(ast::StringLit expr) {
  const auto value = expr.getValue();
  const auto isRaw = expr.getIsRaw();
  if (!value || !isRaw) {
    error(expr, "string literal is missing its value").emit();
    return nullptr;
  }
  const auto *type = ctx.getTypeInterner().getStr();
  const auto *hir = ctx.getBuilder().makeStringLit(*value, *isRaw, type);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

diagnostics::DiagnosticBuilder HirLowerer::error(ast::AstNode node,
                                                 std::string message) {
  const auto range = node.getRange();
  return diagnostics.error(diagnostics::Span{source, range.start, range.end},
                           std::move(message));
}

} // namespace yuzu::hir
