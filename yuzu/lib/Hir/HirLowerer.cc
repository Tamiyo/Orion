#include "yuzu/Hir/HirLowerer.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Hir/Ops/BuiltinOps.h"
#include "yuzu/Hir/Ops/Op.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Util/ErrorHandling.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/FormatVariadic.h>

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

const Op *toHir(ast::UnaryOp op) {
  switch (op) {
  case ast::UnaryOp::Pos:
    return UnaryPosOp::get();
  case ast::UnaryOp::Neg:
    return UnaryNegOp::get();
  case ast::UnaryOp::Not:
    return UnaryNotOp::get();
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

  const auto *hir =
      ctx.getBuilder().makeIdent(ctx.getStringInterner().intern(*name));
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

  // Name binding (ident → decl) and the type happen in the resolve pass.
  const auto *hir = ctx.getBuilder().makeIdentExpr(loweredIdent);
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

  // Record the declared annotation type (if any) in the type slot. The
  // resolve pass reads it as the expected type, checks the initializer
  // against it, then overwrites the slot with the final binding type.
  // Name binding and inference of an un-annotated binding live there too.
  if (const auto annotation = stmt.getType()) {
    ctx.getTypeContext().bind(hir, lowerType(*annotation));
  }
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
  case ast::SyntaxKind::UnaryExpr:
    return lowerUnaryExpr(*ast::UnaryExpr::cast(expr));
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

  // The operator records which op this is; the resolve pass runs it to
  // compute the result type (and check the operands).
  const std::array<const Expr *, 2> args = {loweredLhs, loweredRhs};
  const auto *hir = ctx.getBuilder().makeCallExpr(toHir(*op), args);
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

const Expr *HirLowerer::lowerUnaryExpr(ast::UnaryExpr expr) {
  const auto op = expr.getOp();
  const auto operand = expr.getExpr();
  if (!op || !operand) {
    error(expr, "incomplete unary expression").emit();
    return nullptr;
  }

  const Expr *lowered = lowerExpr(*operand);
  if (!lowered) {
    return nullptr;
  }

  // Fold negation of a numeric literal into the literal itself, so the value
  // (e.g. `-128`) is range-checked as written rather than as its positive
  // magnitude. (`+literal` needs no fold — its magnitude is unchanged.)
  if (*op == ast::UnaryOp::Neg) {
    if (const IntLit *lit = IntLit::cast(lowered)) {
      const auto *hir = ctx.getBuilder().makeIntLit(-lit->getValue());
      ctx.getSourceTable().bind(hir->getId(), expr);
      return hir;
    }
    if (const FloatLit *lit = FloatLit::cast(lowered)) {
      const auto *hir = ctx.getBuilder().makeFloatLit(-lit->getValue());
      ctx.getSourceTable().bind(hir->getId(), expr);
      return hir;
    }
  }

  // Everything else lowers to a call on the matching unary operator.
  const std::array<const Expr *, 1> args = {lowered};
  const auto *hir = ctx.getBuilder().makeCallExpr(toHir(*op), args);
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
  const auto *hir = ctx.getBuilder().makeBoolLit(*value);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const IntLit *HirLowerer::lowerIntLit(ast::IntLit expr) {
  const auto value = expr.getValue();
  if (!value) {
    error(expr, "integer literal is missing its value").emit();
    return nullptr;
  }
  const auto *hir = ctx.getBuilder().makeIntLit(*value);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const FloatLit *HirLowerer::lowerFloatLit(ast::FloatLit expr) {
  const auto value = expr.getValue();
  if (!value) {
    error(expr, "float literal is missing its value").emit();
    return nullptr;
  }
  const auto *hir = ctx.getBuilder().makeFloatLit(*value);
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
  const auto *hir = ctx.getBuilder().makeStringLit(
      ctx.getStringInterner().intern(*decoded), *isRaw);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const Type *HirLowerer::lowerType(ast::TypeExpr type) {
  switch (type.getKind()) {
  case ast::SyntaxKind::NamedType:
    return lowerNamedType(*ast::NamedType::cast(type));
  case ast::SyntaxKind::FuncType:
    error(type, "function types are not supported yet").emit();
    return ctx.getTypeContext().getError();
  case ast::SyntaxKind::RecordType:
    error(type, "record types are not supported yet").emit();
    return ctx.getTypeContext().getError();
  default:
    util::yuzu_unreachable();
  }
}

const Type *HirLowerer::lowerNamedType(ast::NamedType type) {
  auto &types = ctx.getTypeContext();

  const auto ident = type.getName();
  const auto name = ident ? ident->getName() : std::nullopt;
  if (!name) {
    error(type, "type is missing its name").emit();
    return types.getError();
  }

  std::vector<const Type *> args;
  for (const ast::TypeExpr arg : type.getArgs()) {
    args.push_back(lowerType(arg));
  }

  // // `Relation[T]` — the only built-in constructor so far.
  // if (*name == U"Relation") {
  //   if (args.size() != 1) {
  //     error(type, "`Relation` takes exactly one type argument").emit();
  //     return types.getError();
  //   }
  //   return types.getRelation(args[0]);
  // }

  if (!args.empty()) {
    error(
        type,
        llvm::formatv("`{0}` is not a generic type", util::toUtf8(*name)).str())
        .emit();
    return types.getError();
  }

  if (const auto *resolved = types.resolveNamed(*name)) {
    return resolved;
  }
  error(type, llvm::formatv("unknown type `{0}`", util::toUtf8(*name)).str())
      .emit();
  return types.getError();
}

diagnostics::DiagnosticBuilder HirLowerer::error(ast::AstNode node,
                                                 std::string message) {
  const auto range = node.getRange();
  return diagnostics.error(diagnostics::Span{source, range.start, range.end},
                           std::move(message));
}

} // namespace yuzu::hir
