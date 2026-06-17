#include "yuzu/Hir/HirLowerer.h"

#include "yuzu/Ast/Ast.h"

#include <string>
#include <vector>

namespace yuzu::hir {
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

diagnostics::DiagnosticBuilder HirLowerer::error(ast::AstNode node,
                                                 std::string message) {
  const auto range = node.getRange();
  return diagnostics.error(diagnostics::Span{source, range.start, range.end},
                           std::move(message));
}

} // namespace yuzu::hir
