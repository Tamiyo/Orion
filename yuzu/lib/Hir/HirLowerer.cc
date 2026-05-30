#include "yuzu/Hir/HirLowerer.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Hir/Ops/BuiltinOps.h"
#include "yuzu/Hir/Ops/Op.h"
#include "yuzu/Util/ErrorHandling.h"

#include <string>
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
  case ast::BinOp::And:
    return AndOp::get();
  case ast::BinOp::Or:
    return OrOp::get();
  case ast::BinOp::In:
    return InOp::get();
  case ast::BinOp::NotIn:
    return NotInOp::get();
  case ast::BinOp::Pow:
    return PowOp::get();
  case ast::BinOp::Eq:
    return EqOp::get();
  case ast::BinOp::Neq:
    return NeqOp::get();
  case ast::BinOp::Lt:
    return LtOp::get();
  case ast::BinOp::Lte:
    return LteOp::get();
  case ast::BinOp::Gt:
    return GtOp::get();
  case ast::BinOp::Gte:
    return GteOp::get();
  case ast::BinOp::ShiftLeft:
    return ShiftLeftOp::get();
  case ast::BinOp::ShiftRight:
    return ShiftRightOp::get();
    break;
  }
  util::yuzu_unreachable();
}

/// Materialise the cooked value of a string-literal token. `raw` is the
/// full token source, including surrounding quotes (and the leading `r`
/// for raw strings). Returns `nullopt` if the token is too short to be a
/// valid literal; the lexer guarantees backslash-escape pairings, so the
/// `i + 1` read inside the loop is always in bounds.
std::optional<std::u32string> decodeStringLiteral(std::u32string_view raw,
                                                  bool isRaw) {
  if (isRaw) {
    // Raw form: `r"..."`. Strip leading `r"` (2 chars) and trailing `"`.
    if (raw.size() < 3) {
      return std::nullopt;
    }
    return std::u32string(raw.substr(2, raw.size() - 3));
  }

  // Cooked form: `"..."`. Strip surrounding `"` then resolve escapes.
  if (raw.size() < 2) {
    return std::nullopt;
  }
  std::u32string out;
  out.reserve(raw.size() - 2);
  for (std::size_t i = 1; i + 1 < raw.size(); ++i) {
    const char32_t c = raw[i];
    if (c != U'\\') {
      out.push_back(c);
      continue;
    }
    const char32_t e = raw[++i];
    switch (e) {
    case U'n':
      out.push_back(U'\n');
      break;
    case U't':
      out.push_back(U'\t');
      break;
    case U'r':
      out.push_back(U'\r');
      break;
    case U'0':
      out.push_back(U'\0');
      break;
    case U'\\':
      out.push_back(U'\\');
      break;
    case U'"':
      out.push_back(U'"');
      break;
    case U'\'':
      out.push_back(U'\'');
      break;
    default:
      // Unknown escape: keep the backslash + trailing char verbatim so a
      // future diagnostic can point at the offending pair.
      out.push_back(U'\\');
      out.push_back(e);
      break;
    }
  }
  return out;
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

const Ident *HirLowerer::lowerIdent(ast::Ident ident) {
  const auto name = ident.getName();
  if (!name) {
    error(ident, "missing identifier name").emit();
    return nullptr;
  }

  const auto *hir = ctx.getBuilder().makeIdent(std::u32string(*name));
  ctx.getSourceTable().bind(hir->getId(), ident);
  return hir;
}

const IdentExpr *HirLowerer::lowerIdentExpr(ast::IdentExpr identExpr) {
  const auto name = identExpr.getName();
  if (!name) {
    error(identExpr, "missing identifier name").emit();
    return nullptr;
  }

  const Ident *loweredIdent = lowerIdent(*name);
  if (!loweredIdent) {
    return nullptr;
  }

  const Expr *expr = ctx.getSymbolTable().lookup(loweredIdent);
  if (!expr) {
    error(identExpr, "unresolved identifier").emit();
    return nullptr;
  }

  const auto *hir =
      ctx.getBuilder().makeIdentExpr(loweredIdent, expr->getType());

  ctx.getSourceTable().bind(hir->getId(), identExpr);
  return hir;
}

const Stmt *HirLowerer::lowerStmt(ast::Stmt stmt) {
  switch (stmt.getKind()) {
  case ast::SyntaxKind::LetStmt:
    return lowerLetStmt(*ast::LetStmt::cast(stmt));
  case ast::SyntaxKind::ExprStmt:
    return lowerExprStmt(*ast::ExprStmt::cast(stmt));
  default:
    util::yuzu_unreachable();
  }
}

const LetStmt *HirLowerer::lowerLetStmt(ast::LetStmt stmt) {
  const auto name = stmt.getName();
  if (!name) {
    error(stmt, "incomplete let binding").emit();
    return nullptr;
  }

  const Ident *loweredIdent = lowerIdent(*name);
  if (!loweredIdent) {
    return nullptr;
  }

  const auto expr = stmt.getExpr();
  if (!expr) {
    error(stmt, "incomplete expression").emit();
    return nullptr;
  }

  const Expr *loweredExpr = lowerExpr(*expr);
  if (!loweredExpr) {
    return nullptr;
  }

  const auto *hir = ctx.getBuilder().makeLetStmt(loweredIdent, loweredExpr);
  ctx.getSourceTable().bind(hir->getId(), stmt);
  ctx.getSymbolTable().bind(loweredIdent, loweredExpr);
  return hir;
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
  case ast::SyntaxKind::ParenExpr:
    return lowerParenExpr(*ast::ParenExpr::cast(expr));
  case ast::SyntaxKind::IdentExpr:
    return lowerIdentExpr(*ast::IdentExpr::cast(expr));
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

const Expr *HirLowerer::lowerParenExpr(ast::ParenExpr expr) {
  const auto inner = expr.getExpr();
  if (!inner) {
    error(expr, "parenthesized expression is missing its inner expression")
        .emit();
    return nullptr;
  }
  return lowerExpr(*inner);
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
  const auto raw = expr.getValue();
  const auto isRaw = expr.getIsRaw();
  if (!raw || !isRaw) {
    error(expr, "string literal is missing its value").emit();
    return nullptr;
  }
  auto decoded = decodeStringLiteral(*raw, *isRaw);
  if (!decoded) {
    error(expr, "string literal is missing its value").emit();
    return nullptr;
  }
  const auto *type = ctx.getTypeInterner().getStr();
  const auto *hir =
      ctx.getBuilder().makeStringLit(std::move(*decoded), *isRaw, type);
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
