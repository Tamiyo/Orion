#include "yuzu/Hir/Ops/BuiltinOps.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeCoercion.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FormatVariadic.h>

namespace yuzu::hir {
namespace {

/// Standard "operator can't be applied to these operands" diagnostic,
/// anchored across both operands so the caret spans the whole
/// expression. Returns the error type so a resolve can tail-call it:
/// `return unsupportedOperands(args, ctx, "<")`.
const Type *unsupportedOperands(llvm::ArrayRef<const Expr *> args,
                                HirContext &ctx, llvm::StringRef symbol) {
  ctx.error(args,
            llvm::formatv("binary operator `{0}` cannot be applied to "
                          "`{1}` and `{2}`",
                          symbol, asString(args[0]->getType()->getKind()),
                          asString(args[1]->getType()->getKind()))
                .str())
      .emit();
  return ctx.getTypeInterner().getError();
}

} // namespace

//===----------------------------------------------------------------------===//
// Arithmetic — numeric operands coerce to a common type. `+` also
// concatenates two strings.
//===----------------------------------------------------------------------===//

const Type *AddOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (const auto *coerced = coerceTypes(args[0], args[1], ctx)) {
      return coerced;
    }
  }

  if (lhsType->getKind() == TypeKind::Str &&
      rhsType->getKind() == TypeKind::Str) {
    return ctx.getTypeInterner().getStr();
  }

  return unsupportedOperands(args, ctx, "+");
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

  return unsupportedOperands(args, ctx, "-");
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

  return unsupportedOperands(args, ctx, "*");
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

  return unsupportedOperands(args, ctx, "/");
}

const Type *PowOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (const auto *coerced = coerceTypes(args[0], args[1], ctx)) {
      return coerced;
    }
  }

  return unsupportedOperands(args, ctx, "**");
}

//===----------------------------------------------------------------------===//
// Logical — both operands must be `bool`.
//===----------------------------------------------------------------------===//

const Type *AndOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  if (args[0]->getType()->getKind() == TypeKind::Bool &&
      args[1]->getType()->getKind() == TypeKind::Bool) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, "and");
}

const Type *OrOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  if (args[0]->getType()->getKind() == TypeKind::Bool &&
      args[1]->getType()->getKind() == TypeKind::Bool) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, "or");
}

//===----------------------------------------------------------------------===//
// Membership — no container types exist yet, so the only definable form
// is substring containment (`str in str`).
//===----------------------------------------------------------------------===//

const Type *InOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  if (args[0]->getType()->getKind() == TypeKind::Str &&
      args[1]->getType()->getKind() == TypeKind::Str) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, "in");
}

const Type *NotInOp::resolve(llvm::ArrayRef<const Expr *> args,
                             HirContext &ctx) const {
  if (args[0]->getType()->getKind() == TypeKind::Str &&
      args[1]->getType()->getKind() == TypeKind::Str) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, "not in");
}

//===----------------------------------------------------------------------===//
// Equality — operands that coerce to a common type are comparable.
// `coerceTypes` runs for the cast adjustment it records (consumed by
// codegen); its returned type is discarded since the result is `bool`.
//===----------------------------------------------------------------------===//

const Type *EqOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  if (coerceTypes(args[0], args[1], ctx)) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, "==");
}

const Type *NeqOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  if (coerceTypes(args[0], args[1], ctx)) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, "!=");
}

//===----------------------------------------------------------------------===//
// Ordering — numeric (coerced) or string operands; `bool` has no ordering.
//===----------------------------------------------------------------------===//

const Type *LtOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (coerceTypes(args[0], args[1], ctx)) {
      return ctx.getTypeInterner().getBool();
    }
  }

  if (lhsType->getKind() == TypeKind::Str &&
      rhsType->getKind() == TypeKind::Str) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, "<");
}

const Type *LteOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (coerceTypes(args[0], args[1], ctx)) {
      return ctx.getTypeInterner().getBool();
    }
  }

  if (lhsType->getKind() == TypeKind::Str &&
      rhsType->getKind() == TypeKind::Str) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, "<=");
}

const Type *GtOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (coerceTypes(args[0], args[1], ctx)) {
      return ctx.getTypeInterner().getBool();
    }
  }

  if (lhsType->getKind() == TypeKind::Str &&
      rhsType->getKind() == TypeKind::Str) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, ">");
}

const Type *GteOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  const auto *lhsType = args[0]->getType();
  const auto *rhsType = args[1]->getType();

  if (lhsType->isNumeric() && rhsType->isNumeric()) {
    if (coerceTypes(args[0], args[1], ctx)) {
      return ctx.getTypeInterner().getBool();
    }
  }

  if (lhsType->getKind() == TypeKind::Str &&
      rhsType->getKind() == TypeKind::Str) {
    return ctx.getTypeInterner().getBool();
  }

  return unsupportedOperands(args, ctx, ">=");
}

//===----------------------------------------------------------------------===//
// Bit shifts — both operands must be integers. The result is the left
// operand's type; the right operand is only the shift count, so the two
// sides are not coerced together.
//===----------------------------------------------------------------------===//

const Type *ShiftLeftOp::resolve(llvm::ArrayRef<const Expr *> args,
                                 HirContext &ctx) const {
  if (args[0]->getType()->isInt() && args[1]->getType()->isInt()) {
    return args[0]->getType();
  }

  return unsupportedOperands(args, ctx, "<<");
}

const Type *ShiftRightOp::resolve(llvm::ArrayRef<const Expr *> args,
                                  HirContext &ctx) const {
  if (args[0]->getType()->isInt() && args[1]->getType()->isInt()) {
    return args[0]->getType();
  }

  return unsupportedOperands(args, ctx, ">>");
}

} // namespace yuzu::hir
