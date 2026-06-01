#include "yuzu/Hir/Types/TypeInferrer.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/Op.h"
#include "yuzu/Hir/Resolve/HirSymbolTable.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeCoercion.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/FormatVariadic.h>

#include <variant>
#include <vector>

namespace yuzu::hir {
namespace {
// Type of a binding, uniform across the variant arms (all read the side table).
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

// Untyped numeric literals are holes; context pins them, else they default.
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
  auto &types = ctx.getTypeContext();

  // A failed operand poisons the call silently — no cascading diagnostic.
  for (const Expr *arg : n->getArgs()) {
    if (types.typeOf(arg)->getKind() == TypeKind::Error) {
      types.bind(n, types.getError());
      return;
    }
  }
  types.bind(n, n->getOp()->resolve(n->getArgs(), ctx));
}

void TypeInferrer::visitFnCallExpr(const FnCallExpr *n) {
  auto &types = ctx.getTypeContext();
  const Expr *callee = n->getCallee();
  const Type *calleeType = types.typeOf(callee);

  // A failed callee already reported; stay quiet.
  if (calleeType->getKind() == TypeKind::Error) {
    types.bind(n, types.getError());
    return;
  }

  const auto *fn = FuncTy::cast(calleeType);
  if (!fn) {
    ctx.error(callee, llvm::formatv("`{0}` is not callable",
                                    asString(calleeType->getKind()))
                          .str())
        .emit();
    types.bind(n, types.getError());
    return;
  }

  const auto args = n->getArgs();
  const auto params = fn->getParams();
  if (args.size() != params.size()) {
    ctx.error(n, llvm::formatv("expected {0} argument(s), found {1}",
                               params.size(), args.size())
                     .str())
        .emit();
    types.bind(n, fn->getRet());
    return;
  }

  // Per-call substitution of `[T]` markers → fresh holes, so calls don't
  // disturb the stored template.
  llvm::DenseMap<uint32_t, const Type *> subst;

  // Each argument must be assignable to its instantiated parameter.
  for (size_t i = 0; i < args.size(); ++i) {
    const Type *param = instantiate(params[i], subst);
    if (types.unify(types.typeOf(args[i]), param)) {
      continue;
    }
    types.concretize(args[i]);
    if (!coercesTo(args[i], types.resolve(param), ctx)) {
      ctx.error(args[i],
                llvm::formatv("argument of type `{0}` is not assignable to "
                              "parameter of type `{1}`",
                              asString(types.typeOf(args[i])->getKind()),
                              asString(types.resolve(param)->getKind()))
                    .str())
          .emit();
    }
  }

  // Leave the return as a (possibly open) hole so an annotation can pin it.
  types.bind(n, instantiate(fn->getRet(), subst));
}

const Type *
TypeInferrer::instantiate(const Type *type,
                          llvm::DenseMap<uint32_t, const Type *> &subst) {
  if (const auto *tp = TypeParamTy::cast(type)) {
    // Same `[T]` → same fresh hole within this call.
    auto [it, inserted] = subst.try_emplace(tp->getIndex(), nullptr);
    if (inserted) {
      it->second = ctx.getTypeContext().hole(InferKind::General);
    }
    return it->second;
  }

  // Nested function types substitute component-wise; concretes pass through.
  if (const auto *fn = FuncTy::cast(type)) {
    std::vector<const Type *> params;
    params.reserve(fn->getParams().size());
    for (const Type *p : fn->getParams()) {
      params.push_back(instantiate(p, subst));
    }
    return ctx.getTypeContext().getFunc(params,
                                        instantiate(fn->getRet(), subst));
  }

  return type;
}

void TypeInferrer::visitLetStmt(const LetStmt *n) {
  auto &types = ctx.getTypeContext();
  const Type *initType = types.typeOf(n->getExpr());

  if (const Type *declared = resolveAnnotation(n)) {
    types.bind(n, declared);
    // Unify so a literal adopts the annotation (`let x: int32 = 5`); else
    // concretize and widen-coerce (`let x: float64 = 5`).
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
    // No annotation: infer, resolving now so references read a concrete type.
    types.bind(n, types.resolve(initType));
  }

  ctx.getSymbolTable().bind(n->getName(), n);
}

void TypeInferrer::traverseFnStmt(const FnStmt *n) {
  auto &types = ctx.getTypeContext();
  auto &symbols = ctx.getSymbolTable();

  // Bind the name in the enclosing scope so the body can recurse.
  symbols.bind(n->getName(), n);

  // Fresh scope for the type params, value params, and locals.
  const HirScopeGuard guard = symbols.pushScope(HirScopeKind::Fn);

  // Each `[T]` is a rigid `TypeParamTy` in the type namespace, so `: T`
  // resolves.
  for (size_t i = 0; i < n->getTypeParams().size(); ++i) {
    const Ident *tp = n->getTypeParams()[i];
    symbols.bindType(tp->getName(), types.getTypeParam(static_cast<uint32_t>(i),
                                                       tp->getName()));
  }

  // Resolve each parameter's annotation (now `[T]` is visible) and bind it.
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
    symbols.bind(param->getName(), param);
  }

  // Return type: the annotation, else error as a placeholder (no unit type
  // yet) — an undeclared return is left unchecked.
  const Type *declaredReturn = resolveAnnotation(n);
  const Type *retType = declaredReturn ? declaredReturn : types.getError();

  // The signature; a template when generic (`[T]` params are markers).
  types.bind(n, types.getFunc(paramTypes, retType));

  const Type *outerReturn = expectedReturn;
  expectedReturn = declaredReturn;
  visit(n->getBody());
  expectedReturn = outerReturn;
}

void TypeInferrer::visitReturnStmt(const ReturnStmt *returnStmt) {
  // Check only a value `return` inside a function with a declared return type.
  if (expectedReturn == nullptr || returnStmt->getExpr() == nullptr ||
      expectedReturn->getKind() == TypeKind::Error) {
    return;
  }

  auto &types = ctx.getTypeContext();
  const Expr *expr = returnStmt->getExpr();

  // Unify so a literal adopts the return type; else concretize and widen.
  if (types.unify(types.typeOf(expr), expectedReturn)) {
    return;
  }
  types.concretize(expr);
  if (!coercesTo(expr, expectedReturn, ctx)) {
    ctx.error(returnStmt, llvm::formatv("returning `{0}` from a function declared to "
                               "return `{1}`",
                               asString(types.typeOf(expr)->getKind()),
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

const Type *TypeInferrer::resolveType(ast::TypeExpr typeExpr, const HirNode *node) {
  switch (typeExpr.getKind()) {
  case ast::SyntaxKind::NamedType:
    return resolveNamedType(*ast::NamedType::cast(typeExpr), node);
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
    ctx.error(node,
              llvm::formatv("`{0}` is not a generic type", util::toUtf8(*name))
                  .str())
        .emit();
    return types.getError();
  }

  // A `[T]` in scope precedes any global type name.
  if (const Type *param = ctx.getSymbolTable().lookupType(*name)) {
    return param;
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