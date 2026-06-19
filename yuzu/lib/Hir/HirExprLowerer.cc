#include "yuzu/Hir/HirLowerer.h"

#include "yuzu/Diagnostics/DiagnosticBuilder.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirBuilder.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Ops/BuiltinOp.h"

namespace yuzu::hir {
namespace {
BuiltinOp toHir(ast::BinOp op) {
  switch (op) {
  case ast::BinOp::Add:
    return BuiltinOp::Add;
  case ast::BinOp::Sub:
    return BuiltinOp::Sub;
  case ast::BinOp::Mul:
    return BuiltinOp::Mul;
  case ast::BinOp::Div:
    return BuiltinOp::Div;
  case ast::BinOp::And:
    return BuiltinOp::And;
  case ast::BinOp::Or:
    return BuiltinOp::Or;
  case ast::BinOp::In:
    return BuiltinOp::In;
  case ast::BinOp::NotIn:
    return BuiltinOp::NotIn;
  case ast::BinOp::Pow:
    return BuiltinOp::Pow;
  case ast::BinOp::Eq:
    return BuiltinOp::Eq;
  case ast::BinOp::Neq:
    return BuiltinOp::Neq;
  case ast::BinOp::Lt:
    return BuiltinOp::Lt;
  case ast::BinOp::Lte:
    return BuiltinOp::Lte;
  case ast::BinOp::Gt:
    return BuiltinOp::Gt;
  case ast::BinOp::Gte:
    return BuiltinOp::Gte;
  case ast::BinOp::ShiftLeft:
    return BuiltinOp::ShiftLeft;
  case ast::BinOp::ShiftRight:
    return BuiltinOp::ShiftRight;
  }
  util::yuzu_unreachable();
}

BuiltinOp toHir(ast::UnaryOp op) {
  switch (op) {
  case ast::UnaryOp::Pos:
    return BuiltinOp::UnaryPos;
  case ast::UnaryOp::Neg:
    return BuiltinOp::UnaryNeg;
  case ast::UnaryOp::Not:
    return BuiltinOp::UnaryNot;
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

const Expr *HirLowerer::lowerExpr(ast::Expr expr) {
  switch (expr.getKind()) {
  // Literals
  case ast::SyntaxKind::BoolLit:
  case ast::SyntaxKind::IntLit:
  case ast::SyntaxKind::FloatLit:
  case ast::SyntaxKind::StringLit:
    return lowerLiteralExpr(*ast::Literal::cast(expr));

  // Operators
  case ast::SyntaxKind::BinaryExpr:
    return lowerBinaryExpr(*ast::BinaryExpr::cast(expr));
  case ast::SyntaxKind::UnaryExpr:
    return lowerUnaryExpr(*ast::UnaryExpr::cast(expr));
  case ast::SyntaxKind::ParenExpr:
    return lowerParenExpr(*ast::ParenExpr::cast(expr));

  // Names and calls
  case ast::SyntaxKind::IdentExpr:
    return lowerIdentExpr(*ast::IdentExpr::cast(expr));
  case ast::SyntaxKind::CallExpr:
    return lowerCallExpr(*ast::CallExpr::cast(expr));

  // Structs
  case ast::SyntaxKind::StructExpr:
    return lowerStructExpr(*ast::StructExpr::cast(expr));
  case ast::SyntaxKind::FieldAccessExpr:
    return lowerFieldAccessExpr(*ast::FieldAccessExpr::cast(expr));

  // Queries
  case ast::SyntaxKind::FromExpr:
    return lowerFromRel(*ast::FromExpr::cast(expr));
  case ast::SyntaxKind::SelectExpr:
    return lowerSelectRel(*ast::SelectExpr::cast(expr));
  case ast::SyntaxKind::WhereExpr:
    return lowerWhereRel(*ast::WhereExpr::cast(expr));
  case ast::SyntaxKind::DistinctExpr:
    return lowerDistinctRel(*ast::DistinctExpr::cast(expr));
  case ast::SyntaxKind::DropExpr:
    return lowerDropRel(*ast::DropExpr::cast(expr));

  default:
    util::yuzu_unreachable();
  }
}

const Expr *HirLowerer::lowerFieldAccessExpr(ast::FieldAccessExpr expr) {
  const auto base = expr.getBase();
  const auto field = expr.getField();
  if (!base || !field) {
    error(expr, "field access is incomplete").emit();
    return nullptr;
  }
  const Expr *loweredBase = lowerExpr(*base);
  const Ident *loweredField = lowerIdent(*field);
  if (!loweredBase || !loweredField) {
    return nullptr;
  }

  // const auto *hir =
  //     ctx.getBuilder().makeFieldAccessExpr(loweredBase, loweredField);

  // ctx.getSourceTable().bind(hir->getId(), expr);
  // return hir;

  return ctx.build(expr, &HirBuilder::makeFieldAccessExpr, loweredBase,
                   loweredField);
}

const StructFieldInit *
HirLowerer::lowerStructFieldInit(ast::StructFieldInit field) {
  const auto name = field.getName();
  const auto value = field.getValue();
  if (!name || !value) {
    error(field, "struct field initializer is incomplete").emit();
    return nullptr;
  }
  const Ident *loweredName = lowerIdent(*name);
  const Expr *loweredValue = lowerExpr(*value);
  if (!loweredName || !loweredValue) {
    return nullptr;
  }

  const auto *hir =
      ctx.getBuilder().makeStructFieldInit(loweredName, loweredValue);
  ctx.getSourceTable().bind(hir->getId(), field);
  return hir;
}

const Expr *HirLowerer::lowerStructExpr(ast::StructExpr expr) {
  const auto name = expr.getName();
  if (!name) {
    error(expr, "struct literal is missing its type name").emit();
    return nullptr;
  }
  const Ident *loweredName = lowerIdent(*name);
  if (!loweredName) {
    return nullptr;
  }

  std::vector<const StructFieldInit *> fields;
  for (const ast::StructFieldInit field : expr.getFields()) {
    if (const StructFieldInit *lowered = lowerStructFieldInit(field)) {
      fields.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeStructExpr(loweredName, fields);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
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

const Expr *HirLowerer::lowerCallExpr(ast::CallExpr expr) {
  const auto callee = expr.getCallee();
  if (!callee) {
    error(expr, "call is missing its callee").emit();
    return nullptr;
  }
  const Expr *loweredCallee = lowerExpr(*callee);
  if (!loweredCallee) {
    return nullptr;
  }

  std::vector<const Expr *> args;
  if (const auto argList = expr.getArgs()) {
    for (const ast::Expr arg : argList->getArgs()) {
      if (const Expr *lowered = lowerExpr(arg)) {
        args.push_back(lowered);
      }
    }
  }

  const auto *hir = ctx.getBuilder().makeFuncCallExpr(loweredCallee, args);
  ctx.getSourceTable().bind(hir->getId(), expr);
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
} // namespace yuzu::hir