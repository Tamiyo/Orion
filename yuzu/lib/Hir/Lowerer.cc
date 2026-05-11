#include "yuzu/Hir/Lowerer.h"

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

const Root *Lowerer::lower(const ast::Root &root) {
  std::vector<const Stmt *> stmts;
  for (const ast::Stmt &stmt : root.getStmts()) {
    if (const Stmt *lowered = lowerStmt(stmt)) {
      stmts.push_back(lowered);
    }
  }
  return builder.makeRoot(stmts);
}

const Stmt *Lowerer::lowerStmt(const ast::Stmt & /*stmt*/) {
  util::yuzu_unreachable();
}

const Expr *Lowerer::lowerExpr(const ast::Expr &expr) {
  switch (expr.getKind()) {
  case ast::SyntaxKind::BinaryExpr:
    return lowerBinaryExpr(*ast::BinaryExpr::cast(expr));
  case ast::SyntaxKind::LiteralExpr:
    return lowerLiteralExpr(*ast::LiteralExpr::cast(expr));
  default:
    util::yuzu_unreachable();
  }
}

const Expr *Lowerer::lowerBinaryExpr(const ast::BinaryExpr &expr) {
  const auto lhs = expr.getLhs();
  const auto op = expr.getOp();
  const auto rhs = expr.getRhs();
  if (!lhs || !op || !rhs) {
    error(expr, "incomplete binary expression").emit();
    return nullptr;
  }
  const Expr *loweredLhs = lowerExpr(*lhs);
  const Expr *loweredRhs = lowerExpr(*rhs);
  if (!loweredLhs || !loweredRhs) {
    return nullptr;
  }
  return builder.makeBinaryExpr(loweredLhs, toHir(*op), loweredRhs);
}

const LiteralExpr *
Lowerer::lowerLiteralExpr(const ast::LiteralExpr & /*expr*/) {
  return builder.makeLiteralExpr();
}

diagnostics::DiagnosticBuilder Lowerer::error(const ast::AstNode &node,
                                              std::string message) {
  const auto range = node.getRange();
  return diagnostics.error(diagnostics::Span{source, range.start, range.end},
                           std::move(message));
}

} // namespace yuzu::hir
