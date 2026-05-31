#include "yuzu/Hir/Types/TypeInferrer.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/Op.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeCoercion.h"

#include <llvm/Support/FormatVariadic.h>

namespace yuzu::hir {

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
  const LetStmt *decl = ctx.getSymbolTable().lookup(n->getName());
  if (!decl) {
    ctx.error(n, "unresolved identifier").emit();
    ctx.getTypeContext().bind(n, ctx.getTypeContext().getError());
    return;
  }
  ctx.getTypeContext().bind(n, ctx.getTypeContext().typeOf(decl));
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

  // Lowering left the declared annotation type in the slot, or nothing.
  if (const Type *declared = types.typeOf(n)) {
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

} // namespace yuzu::hir