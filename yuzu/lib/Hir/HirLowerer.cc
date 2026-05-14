#include "yuzu/Hir/HirLowerer.h"

#include "yuzu/Util/ErrorHandling.h"

#include <vector>

namespace yuzu::hir {
namespace {
BinOp toHir(ast::BinOp op) {
  switch (op) {
  case ast::BinOp::Add:
    return BinOp::Add;
  case ast::BinOp::Sub:
    return BinOp::Sub;
  case ast::BinOp::Mul:
    return BinOp::Mul;
  case ast::BinOp::Div:
    return BinOp::Div;
  }
  util::yuzu_unreachable();
}
} // namespace

const Root *HirLowerer::lower(const ast::Root &root) {
  std::vector<const Stmt *> stmts;
  for (const ast::Stmt &stmt : root.getStmts()) {
    if (const Stmt *lowered = lowerStmt(stmt)) {
      stmts.push_back(lowered);
    }
  }
  const auto *hir = builder.makeRoot(stmts);
  sourceMap.bind(hir->getId(), root);
  return hir;
}

const Stmt *HirLowerer::lowerStmt(const ast::Stmt &stmt) {
  switch (stmt.getKind()) {
  case ast::SyntaxKind::ExprStmt:
    return lowerExprStmt(*ast::ExprStmt::cast(stmt));
  default:
    util::yuzu_unreachable();
  }
}

const Stmt *HirLowerer::lowerExprStmt(const ast::ExprStmt &stmt) {
  const auto expr = stmt.getExpr();
  if (!expr) {
    error(stmt, "incomplete statement").emit();
    return nullptr;
  }
  const Expr *loweredExpr = lowerExpr(*expr);
  if (!loweredExpr) {
    return nullptr;
  }
  const auto *hir = builder.makeExprStmt(loweredExpr);
  sourceMap.bind(hir->getId(), stmt);
  return hir;
}

const Expr *HirLowerer::lowerExpr(const ast::Expr &expr) {
  switch (expr.getKind()) {
  case ast::SyntaxKind::BinaryExpr:
    return lowerBinaryExpr(*ast::BinaryExpr::cast(expr));
  case ast::SyntaxKind::LiteralExpr:
    return lowerLiteralExpr(*ast::LiteralExpr::cast(expr));
  default:
    util::yuzu_unreachable();
  }
}

const Expr *HirLowerer::lowerBinaryExpr(const ast::BinaryExpr &expr) {
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
  const auto *hir = builder.makeBinaryExpr(loweredLhs, toHir(*op), loweredRhs);
  sourceMap.bind(hir->getId(), expr);
  return hir;
}

const LiteralExpr *HirLowerer::lowerLiteralExpr(const ast::LiteralExpr &expr) {
  const auto value = expr.getValue();
  if (!value) {
    error(expr, "literal is missing its value").emit();
    return nullptr;
  }
  const auto *hir = builder.makeLiteralExpr(*value);
  sourceMap.bind(hir->getId(), expr);
  return hir;
}

diagnostics::DiagnosticBuilder HirLowerer::error(const ast::AstNode &node,
                                              std::string message) {
  const auto range = node.getRange();
  return diagnostics.error(diagnostics::Span{source, range.start, range.end},
                           std::move(message));
}

diagnostics::DiagnosticBuilder HirLowerer::error(const HirNode *node,
                                              std::string message) {
  // TODO: climb-to-parent fallback once HIR carries parent pointers. For
  // synthetic nodes (no AST origin) the diagnostic lands on a zero-width
  // span at offset 0 — flag for the developer but doesn't crash.
  if (const auto origin = sourceMap.get(node->getId())) {
    return error(*origin, std::move(message));
  }
  return diagnostics.error(diagnostics::Span{source, 0, 0}, std::move(message));
}

} // namespace yuzu::hir
