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
  // Declarations
  case ast::SyntaxKind::StructStmt:
    return lowerStructStmt(*ast::StructStmt::cast(stmt));
  case ast::SyntaxKind::TableStmt:
    return lowerTableStmt(*ast::TableStmt::cast(stmt));
  case ast::SyntaxKind::FuncStmt:
    return lowerFuncStmt(*ast::FuncStmt::cast(stmt));

  // Bindings
  case ast::SyntaxKind::LetStmt:
    return lowerLetStmt(*ast::LetStmt::cast(stmt));
  case ast::SyntaxKind::AssignStmt:
    return lowerAssignStmt(*ast::AssignStmt::cast(stmt));

  // Control flow
  case ast::SyntaxKind::BlockStmt:
    return lowerBlockStmt(*ast::BlockStmt::cast(stmt));
  case ast::SyntaxKind::ReturnStmt:
    return lowerReturnStmt(*ast::ReturnStmt::cast(stmt));

  // Expression statement
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

  // The annotation (if any) lowers to a HIR type-expr; the type pass
  // resolves it against scope and checks the initializer.
  const TypeAnnotation *annotation = nullptr;
  if (const auto type = stmt.getTypeAnnotation()) {
    annotation = lowerTypeAnnotation(*type);
  }

  const Mutability mutability = stmt.getMutability() == ast::Mutability::Mutable
                                    ? Mutability::Mutable
                                    : Mutability::Immutable;

  const auto *hir = ctx.getBuilder().makeLetStmt(loweredIdent, mutability,
                                                 annotation, loweredExpr);
  ctx.getSourceTable().bind(hir->getId(), stmt);
  return hir;
}

const Param *HirLowerer::lowerParam(ast::Param param) {
  const auto name = param.getName();
  if (!name) {
    error(param, "parameter is missing its name").emit();
    return nullptr;
  }

  const Ident *loweredName = lowerIdent(*name);
  if (!loweredName) {
    return nullptr;
  }

  const TypeAnnotation *annotation = nullptr;
  if (const auto type = param.getType()) {
    annotation = lowerTypeAnnotation(*type);
  }

  const auto *hir = ctx.getBuilder().makeParam(loweredName, annotation);
  ctx.getSourceTable().bind(hir->getId(), param);
  return hir;
}

const TypeAnnotation *
HirLowerer::lowerTypeAnnotation(ast::TypeAnnotation type) {
  switch (type.getTypeAnnotationKind()) {
  case ast::TypeAnnotationKind::NamedTypeAnnotation:
    return lowerNamedTypeAnnotation(*ast::NamedTypeAnnotation::cast(type));
  case ast::TypeAnnotationKind::FuncTypeAnnotation:
    return lowerFuncTypeAnnotation(*ast::FuncTypeAnnotation::cast(type));
  case ast::TypeAnnotationKind::RecordType:
    error(type, "record types are not supported yet").emit();
    return nullptr;
  }
}

const NamedTypeAnnotation *
HirLowerer::lowerNamedTypeAnnotation(ast::NamedTypeAnnotation type) {
  const auto name = type.getName();
  if (!name) {
    error(type, "type is missing its name").emit();
    return nullptr;
  }

  const Ident *loweredName = lowerIdent(*name);
  if (!loweredName) {
    return nullptr;
  }

  std::vector<const TypeAnnotation *> args;
  for (const ast::TypeAnnotation &arg : type.getArgs()) {
    if (const TypeAnnotation *lowered = lowerTypeAnnotation(arg)) {
      args.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeNamedTypeAnnotation(loweredName, args);
  ctx.getSourceTable().bind(hir->getId(), type);
  return hir;
}

const FuncTypeAnnotation *
HirLowerer::lowerFuncTypeAnnotation(ast::FuncTypeAnnotation type) {
  std::vector<const TypeAnnotation *> params;
  if (const auto paramList = type.getParams()) {
    for (const ast::TypeAnnotation &param : paramList->getParams()) {
      if (const TypeAnnotation *lowered = lowerTypeAnnotation(param)) {
        params.push_back(lowered);
      }
    }
  }

  const auto result = type.getResult();
  if (!result) {
    error(type, "function type is missing its result type").emit();
    return nullptr;
  }
  const TypeAnnotation *loweredResult = lowerTypeAnnotation(*result);
  if (!loweredResult) {
    return nullptr;
  }

  const auto *hir =
      ctx.getBuilder().makeFuncTypeAnnotation(params, loweredResult);
  ctx.getSourceTable().bind(hir->getId(), type);
  return hir;
}

const BlockStmt *HirLowerer::lowerBlockStmt(ast::BlockStmt stmt) {
  std::vector<const Stmt *> stmts;
  for (const ast::Stmt &s : stmt.getStmts()) {
    if (const Stmt *lowered = lowerStmt(s)) {
      stmts.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeBlockStmt(stmts);
  ctx.getSourceTable().bind(hir->getId(), stmt);
  return hir;
}

const ReturnStmt *HirLowerer::lowerReturnStmt(ast::ReturnStmt stmt) {
  // A bare `return` has no operand (null expr).
  const Expr *loweredExpr = nullptr;
  if (const auto expr = stmt.getExpr()) {
    loweredExpr = lowerExpr(*expr);
    if (!loweredExpr) {
      return nullptr;
    }
  }

  const auto *hir = ctx.getBuilder().makeReturnStmt(loweredExpr);
  ctx.getSourceTable().bind(hir->getId(), stmt);
  return hir;
}

const TraitRef *HirLowerer::lowerTraitRef(ast::TraitRef traitRef) {
  const auto name = traitRef.getName();
  if (!name) {
    error(traitRef, "trait bound is missing its name").emit();
    return nullptr;
  }
  const Ident *loweredName = lowerIdent(*name);
  if (!loweredName) {
    return nullptr;
  }
  const auto *hir = ctx.getBuilder().makeTraitRef(loweredName);
  ctx.getSourceTable().bind(hir->getId(), traitRef);
  return hir;
}

const TypeBound *HirLowerer::lowerTypeBound(ast::TypeBound typeBound) {
  const auto subject = typeBound.getSubject();
  if (!subject) {
    error(typeBound, "bound is missing its type parameter").emit();
    return nullptr;
  }
  const Ident *loweredSubject = lowerIdent(*subject);
  if (!loweredSubject) {
    return nullptr;
  }

  std::vector<const TraitRef *> traits;
  for (const ast::TraitRef &traitRef : typeBound.getTraits()) {
    if (const TraitRef *lowered = lowerTraitRef(traitRef)) {
      traits.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeTypeBound(loweredSubject, traits);
  ctx.getSourceTable().bind(hir->getId(), typeBound);
  return hir;
}

const FuncStmt *HirLowerer::lowerFuncStmt(ast::FuncStmt stmt) {
  const auto name = stmt.getName();
  if (!name) {
    error(stmt, "function is missing its name").emit();
    return nullptr;
  }

  const Ident *loweredName = lowerIdent(*name);
  if (!loweredName) {
    return nullptr;
  }

  std::vector<const Ident *> typeParams;
  for (const ast::TypeParam &tp : stmt.getTypeParams()) {
    const auto tpName = tp.getName();
    if (!tpName) {
      continue;
    }
    if (const Ident *lowered = lowerIdent(*tpName)) {
      typeParams.push_back(lowered);
    }
  }

  std::vector<const Param *> params;
  for (const ast::Param &p : stmt.getParams()) {
    if (const Param *lowered = lowerParam(p)) {
      params.push_back(lowered);
    }
  }

  const auto body = stmt.getBody();
  if (!body) {
    error(stmt, "function is missing its body").emit();
    return nullptr;
  }
  const BlockStmt *loweredBody = lowerBlockStmt(*body);

  const TypeAnnotation *returnType = nullptr;
  if (const auto result = stmt.getResult()) {
    returnType = lowerTypeAnnotation(*result);
  }

  std::vector<const TypeBound *> bounds;
  for (const ast::TypeBound &bound : stmt.getBounds()) {
    if (const TypeBound *lowered = lowerTypeBound(bound)) {
      bounds.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeFuncStmt(
      loweredName, typeParams, params, returnType, bounds, loweredBody);
  ctx.getSourceTable().bind(hir->getId(), stmt);
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

const Stmt *HirLowerer::lowerAssignStmt(ast::AssignStmt stmt) {
  const auto target = stmt.getTarget();
  if (!target) {
    error(stmt, "assignment is missing its target").emit();
    return nullptr;
  }
  const Expr *loweredTarget = lowerExpr(*target);
  if (!loweredTarget) {
    return nullptr;
  }

  const auto value = stmt.getValue();
  if (!value) {
    error(stmt, "assignment is missing its value").emit();
    return nullptr;
  }
  const Expr *loweredValue = lowerExpr(*value);
  if (!loweredValue) {
    return nullptr;
  }

  const auto *hir =
      ctx.getBuilder().makeAssignStmt(loweredTarget, loweredValue);
  ctx.getSourceTable().bind(hir->getId(), stmt);
  return hir;
}

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
  case ast::SyntaxKind::StructLitExpr:
    return lowerStructLitExpr(*ast::StructLitExpr::cast(expr));
  case ast::SyntaxKind::FieldAccessExpr:
    return lowerFieldAccessExpr(*ast::FieldAccessExpr::cast(expr));

  // Queries
  case ast::SyntaxKind::FromExpr:
    return lowerFromExpr(*ast::FromExpr::cast(expr));
  case ast::SyntaxKind::SelectExpr:
    return lowerSelectExpr(*ast::SelectExpr::cast(expr));

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

const Expr *HirLowerer::lowerFromExpr(ast::FromExpr expr) {
  const auto relation = expr.getRelation();
  if (!relation) {
    error(expr, "`from` is missing its relation").emit();
    return nullptr;
  }
  const Ident *loweredRelation = lowerIdent(*relation);
  if (!loweredRelation) {
    return nullptr;
  }

  // The alias is optional (`from t` vs `from t e`).
  const Ident *loweredAlias = nullptr;
  if (const auto alias = expr.getAlias()) {
    loweredAlias = lowerIdent(*alias);
  }

  const auto *hir =
      ctx.getBuilder().makeFromExpr(loweredRelation, loweredAlias);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const SelectItem *HirLowerer::lowerSelectItem(ast::SelectItem item) {
  const auto astExpr = item.getExpr();
  if (!astExpr) {
    error(item, "select item is missing its expression").emit();
    return nullptr;
  }
  const Expr *loweredExpr = lowerExpr(*astExpr);
  if (!loweredExpr) {
    return nullptr;
  }

  const Ident *loweredAlias = nullptr;
  if (const auto alias = item.getAlias()) {
    loweredAlias = lowerIdent(*alias);
  }

  const auto *hir = ctx.getBuilder().makeSelectItem(loweredExpr, loweredAlias);
  ctx.getSourceTable().bind(hir->getId(), item);
  return hir;
}

const Expr *HirLowerer::lowerSelectExpr(ast::SelectExpr expr) {
  const auto input = expr.getInput();
  if (!input) {
    error(expr, "`select` is missing its input relation").emit();
    return nullptr;
  }
  const Expr *loweredInput = lowerExpr(*input);
  if (!loweredInput) {
    return nullptr;
  }

  std::vector<const SelectItem *> items;
  for (const ast::SelectItem item : expr.getItems()) {
    if (const SelectItem *lowered = lowerSelectItem(item)) {
      items.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeSelectExpr(loweredInput, items);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
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

  const auto *hir =
      ctx.getBuilder().makeFieldAccessExpr(loweredBase, loweredField);

  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const StructLitField *
HirLowerer::lowerStructLitField(ast::StructLitField field) {
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
      ctx.getBuilder().makeStructLitField(loweredName, loweredValue);
  ctx.getSourceTable().bind(hir->getId(), field);
  return hir;
}

const Expr *HirLowerer::lowerStructLitExpr(ast::StructLitExpr expr) {
  const auto name = expr.getName();
  if (!name) {
    error(expr, "struct literal is missing its type name").emit();
    return nullptr;
  }
  const Ident *loweredName = lowerIdent(*name);
  if (!loweredName) {
    return nullptr;
  }

  std::vector<const StructLitField *> fields;
  for (const ast::StructLitField field : expr.getFields()) {
    if (const StructLitField *lowered = lowerStructLitField(field)) {
      fields.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeStructLitExpr(loweredName, fields);
  ctx.getSourceTable().bind(hir->getId(), expr);
  return hir;
}

const StructFieldDecl *
HirLowerer::lowerStructFieldDecl(ast::StructFieldDecl field) {
  const auto name = field.getName();
  const auto type = field.getType();
  if (!name || !type) {
    error(field, "struct field is incomplete").emit();
    return nullptr;
  }
  const Ident *loweredName = lowerIdent(*name);
  const TypeAnnotation *loweredType = lowerTypeAnnotation(*type);
  if (!loweredName || !loweredType) {
    return nullptr;
  }

  const auto *hir =
      ctx.getBuilder().makeStructFieldDecl(loweredName, loweredType);
  ctx.getSourceTable().bind(hir->getId(), field);
  return hir;
}

const Stmt *HirLowerer::lowerStructStmt(ast::StructStmt stmt) {
  const auto name = stmt.getName();
  if (!name) {
    error(stmt, "struct is missing its name").emit();
    return nullptr;
  }
  const Ident *loweredName = lowerIdent(*name);
  if (!loweredName) {
    return nullptr;
  }

  std::vector<const StructFieldDecl *> fields;
  for (const ast::StructFieldDecl field : stmt.getFields()) {
    if (const StructFieldDecl *lowered = lowerStructFieldDecl(field)) {
      fields.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeStructStmt(loweredName, fields);
  ctx.getSourceTable().bind(hir->getId(), stmt);
  return hir;
}

const Stmt *HirLowerer::lowerTableStmt(ast::TableStmt stmt) {
  const auto name = stmt.getName();
  if (!name) {
    error(stmt, "table is missing its name").emit();
    return nullptr;
  }
  const Ident *loweredName = lowerIdent(*name);
  if (!loweredName) {
    return nullptr;
  }

  // Named form (`= Employee`) carries a row-struct ident; inline form
  // (`= { ... }`) carries the fields instead. Exactly one is present.
  const Ident *loweredRowStruct = nullptr;
  if (const auto rowStruct = stmt.getRowStruct()) {
    loweredRowStruct = lowerIdent(*rowStruct);
  }

  std::vector<const StructFieldDecl *> inlineFields;
  for (const ast::StructFieldDecl field : stmt.getInlineFields()) {
    if (const StructFieldDecl *lowered = lowerStructFieldDecl(field)) {
      inlineFields.push_back(lowered);
    }
  }

  const auto *hir = ctx.getBuilder().makeTableStmt(
      loweredName, loweredRowStruct, inlineFields);
  ctx.getSourceTable().bind(hir->getId(), stmt);
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

diagnostics::DiagnosticBuilder HirLowerer::error(ast::AstNode node,
                                                 std::string message) {
  const auto range = node.getRange();
  return diagnostics.error(diagnostics::Span{source, range.start, range.end},
                           std::move(message));
}

} // namespace yuzu::hir
