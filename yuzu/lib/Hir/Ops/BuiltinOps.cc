#include "yuzu/Hir/Ops/BuiltinOps.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeCoercion.h"

#include <llvm/Support/FormatVariadic.h>

namespace yuzu::hir {
const Type *AddOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  // <numeric> + <numeric> = <numeric>
  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (const auto *coerced = coerceTypes(args[0], args[1], ctx)) {
      return coerced;
    }
  }

  // <str> + <str> = <str>
  if (lhsType->getKind() == TypeKind::Str &&
      rhsType->getKind() == TypeKind::Str) {
    return ctx.getTypeInterner().getStr();
  }

  ctx.error(args, llvm::formatv("binary operator `+` cannot be applied to "
                                "`{0}` and `{1}`",
                                asString(lhsType->getKind()),
                                asString(rhsType->getKind()))
                      .str())
      .emit();
  return ctx.getTypeInterner().getError();
}

const Type *SubOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (const auto *coerced = coerceTypes(args[0], args[1], ctx)) {
      return coerced;
    }
  }

  ctx.error(args, llvm::formatv("binary operator `-` cannot be applied to "
                                "`{0}` and `{1}`",
                                asString(lhsType->getKind()),
                                asString(rhsType->getKind()))
                      .str())
      .emit();
  return ctx.getTypeInterner().getError();
}

const Type *MulOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (const auto *coerced = coerceTypes(args[0], args[1], ctx)) {
      return coerced;
    }
  }

  ctx.error(args, llvm::formatv("binary operator `*` cannot be applied to "
                                "`{0}` and `{1}`",
                                asString(lhsType->getKind()),
                                asString(rhsType->getKind()))
                      .str())
      .emit();
  return ctx.getTypeInterner().getError();
}

const Type *DivOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (const auto *coerced = coerceTypes(args[0], args[1], ctx)) {
      return coerced;
    }
  }

  ctx.error(args, llvm::formatv("binary operator `/` cannot be applied to "
                                "`{0}` and `{1}`",
                                asString(lhsType->getKind()),
                                asString(rhsType->getKind()))
                      .str())
      .emit();
  return ctx.getTypeInterner().getError();
}

} // namespace yuzu::hir
