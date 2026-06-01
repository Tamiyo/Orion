#include "yuzu/Hir/Types/TypeInferrer.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/Op.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeCoercion.h"
#include "yuzu/Hir/Resolve/HirSymbolTable.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/FormatVariadic.h>

#include <variant>
#include <vector>

namespace yuzu::hir {
namespace {
// Every binding's type lives in the side table, so each variant arm is
// handled identically — a generic visitor keeps it exhaustive (a new arm
// compiles without change) without per-kind branches.
const Type *typeOf(const HirScope::LookupResult &result, HirContext &ctx) {
  if (!result) {
    return nullptr;
  }
  return std::visit(
      [&](const auto *decl) { return ctx.getTypeContext().typeOf(decl); },
      *result);
}
} // namespace

void TypeInferrer::visitBoolLit(const BoolLit *n) {
  ctx.getTypeContext().bind(n, ctx.getTypeContext().getBool());
}

void TypeInferrer::visitStringLit(const StringLit *n) {
  ctx.getTypeContext().bind(n, ctx.getTypeContext().getStr());
}

// Untyped numeric literals are holes the surrounding context pins down (a
// `let` annotation, an operand) or that default during concretize.
void TypeInferrer::visitIntLit(const IntLit *n) {
  ctx.getTypeContext().bind(n, ctx.getTypeContext().hole(InferKind::Int));
}

void TypeInferrer::visitFloatLit(const FloatLit *n) {
  ctx.getTypeContext().bind(n, ctx.getTypeContext().hole(InferKind::Float));
}

void TypeInferrer::visitIdentExpr(const IdentExpr *n) {
  const auto res = ctx.getSymbolTable().lookup(n->getName());

  if (!res) {
    ctx.error(n, "unresolved identifier").emit();
    ctx.getTypeContext().bind(n, ctx.getTypeContext().getError());
    return;
  }

  ctx.getTypeContext().bind(n, typeOf(res, ctx));
}

void TypeInferrer::visitCallExpr(const CallExpr *n) {
  // An operand that already failed poisons the call silently — its own
  // diagnostic is enough; don't cascade a second one from the operator.
  for (const Expr *arg : n->getArgs()) {
    if (ctx.getTypeContext().typeOf(arg)->getKind() == TypeKind::Error) {
      ctx.getTypeContext().bind(n, ctx.getTypeContext().getError());
      return;
    }
  }
  ctx.getTypeContext().bind(n, n->getOp()->resolve(n->getArgs(), ctx));
}

void TypeInferrer::visitLetStmt(const LetStmt *n) {
  auto &types = ctx.getTypeContext();
  const Type *initType = types.typeOf(n->getExpr());

  // Resolve the annotation (if any) against scope. The binding's own type is
  // then recorded so references read it back.
  if (const Type *declared = resolveAnnotation(n)) {
    types.bind(n, declared);
    // Unify so an untyped literal adopts the annotation directly
    // (`let x: int32 = 5`); else concretize the initializer and try a
    // widening coercion (`let x: float64 = 5`).
    if (declared->getKind() != TypeKind::Error &&
        !types.unify(initType, declared)) {
      types.concretize(n->getExpr());
      if (!coercesTo(n->getExpr(), declared, ctx)) {
        ctx.error(n, llvm::formatv(
                         "value of type `{0}` is not assignable to `{1}`",
                         asString(types.typeOf(n->getExpr())->getKind()),
                         asString(declared->getKind()))
                         .str())
            .emit();
      }
    }
  } else {
    // No annotation: infer, resolving now so later references read a
    // concrete type rather than a hole this binding still owns.
    types.bind(n, types.resolve(initType));
  }

  ctx.getSymbolTable().bind(n->getName(), n);
}

void TypeInferrer::traverseFnStmt(const FnStmt *n) {
  auto &types = ctx.getTypeContext();

  // Resolve each parameter's annotation, recording it as the param's type
  // (an unannotated param defaults to the error type so the signature stays
  // well-formed and downstream just propagates the existing diagnostic).
  std::vector<const Type *> paramTypes;
  paramTypes.reserve(n->getParams().size());
  for (const Param *param : n->getParams()) {
    const Type *paramType = resolveAnnotation(param);
    if (!paramType) {
      ctx.error(param, "parameter is missing a type annotation").emit();
      paramType = types.getError();
    }
    types.bind(param, paramType);
    paramTypes.push_back(paramType);
  }

  // Return type: the annotation if present. A bare `fn f() { ... }` has no
  // declared return; until a real unit type exists it gets the error type as
  // a placeholder and its returns are not checked.
  const Type *declaredReturn = resolveAnnotation(n);
  const Type *retType = declaredReturn ? declaredReturn : types.getError();

  // The function's own type is its signature; bind the name in the enclosing
  // scope *before* the body so the body can recurse (`fn f() { ... f() }`).
  types.bind(n, types.getFunc(paramTypes, retType));
  ctx.getSymbolTable().bind(n->getName(), n);

  // Params and locals live in a fresh function scope, popped when `guard`
  // leaves this block. `return` statements check against this function's
  // declared return type; save/restore handles nested functions.
  const HirScopeGuard guard = ctx.getSymbolTable().pushScope(HirScopeKind::Fn);
  for (const Param *param : n->getParams()) {
    ctx.getSymbolTable().bind(param->getName(), param);
  }

  const Type *outerReturn = expectedReturn;
  expectedReturn = declaredReturn;
  visit(n->getBody());
  expectedReturn = outerReturn;
}

void TypeInferrer::visitReturnStmt(const ReturnStmt *n) {
  // Only check when inside a function with a declared return type and the
  // `return` carries a value. (`return;` / unannotated functions wait for a
  // real unit type.)
  if (expectedReturn == nullptr || n->getExpr() == nullptr ||
      expectedReturn->getKind() == TypeKind::Error) {
    return;
  }

  auto &types = ctx.getTypeContext();
  const Expr *value = n->getExpr();

  // Unify so an untyped literal adopts the return type (`-> int32 { return 5 }`
  // pins `5` to int32); else concretize and try a widening coercion.
  if (types.unify(types.typeOf(value), expectedReturn)) {
    return;
  }
  types.concretize(value);
  if (!coercesTo(value, expectedReturn, ctx)) {
    ctx.error(n, llvm::formatv("returning `{0}` from a function declared to "
                               "return `{1}`",
                               asString(types.typeOf(value)->getKind()),
                               asString(expectedReturn->getKind()))
                     .str())
        .emit();
  }
}

const Type *TypeInferrer::resolveAnnotation(const HirNode *node) {
  const auto *annotation = ctx.getTypeAnnotations().get(node->getId());
  if (!annotation) {
    return nullptr;
  }
  return resolveType(*annotation, node);
}

const Type *TypeInferrer::resolveType(ast::TypeExpr type, const HirNode *node) {
  switch (type.getKind()) {
  case ast::SyntaxKind::NamedType:
    return resolveNamedType(*ast::NamedType::cast(type), node);
  case ast::SyntaxKind::FuncType:
    ctx.error(node, "function types are not supported yet").emit();
    return ctx.getTypeContext().getError();
  case ast::SyntaxKind::RecordType:
    ctx.error(node, "record types are not supported yet").emit();
    return ctx.getTypeContext().getError();
  default:
    util::yuzu_unreachable();
  }
}

const Type *TypeInferrer::resolveNamedType(ast::NamedType type,
                                           const HirNode *node) {
  auto &types = ctx.getTypeContext();

  const auto ident = type.getName();
  const auto name = ident ? ident->getName() : std::nullopt;
  if (!name) {
    ctx.error(node, "type is missing its name").emit();
    return types.getError();
  }

  if (!type.getArgs().empty()) {
    ctx.error(node, llvm::formatv("`{0}` is not a generic type",
                                  util::toUtf8(*name))
                        .str())
        .emit();
    return types.getError();
  }

  if (const auto *resolved = types.resolveNamed(*name)) {
    return resolved;
  }
  ctx.error(node,
            llvm::formatv("unknown type `{0}`", util::toUtf8(*name)).str())
      .emit();
  return types.getError();
}

} // namespace yuzu::hir