#include "yuzu/Hir/HirLowerer.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Util/ErrorHandling.h"

#include <string>

namespace yuzu::hir {
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

  return ctx.build(field, &HirBuilder::makeStructFieldDecl, loweredName,
                   loweredType);
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

  return ctx.build(stmt, &HirBuilder::makeStructStmt, loweredName, fields);
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

  return ctx.build(stmt, &HirBuilder::makeTableStmt, loweredName,
                   loweredRowStruct, inlineFields);
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

  return ctx.build(stmt, &HirBuilder::makeFuncStmt, loweredName, typeParams,
                   params, returnType, bounds, loweredBody);
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

  return ctx.build(stmt, &HirBuilder::makeExprStmt, loweredExpr);
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

  return ctx.build(stmt, &HirBuilder::makeAssignStmt, loweredTarget,
                   loweredValue);
}

const BlockStmt *HirLowerer::lowerBlockStmt(ast::BlockStmt stmt) {
  std::vector<const Stmt *> stmts;
  for (const ast::Stmt &s : stmt.getStmts()) {
    if (const Stmt *lowered = lowerStmt(s)) {
      stmts.push_back(lowered);
    }
  }

  return ctx.build(stmt, &HirBuilder::makeBlockStmt, stmts);
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

  return ctx.build(stmt, &HirBuilder::makeReturnStmt, loweredExpr);
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

  return ctx.build(stmt, &HirBuilder::makeLetStmt, loweredIdent, mutability,
                   annotation, loweredExpr);
}

} // namespace yuzu::hir