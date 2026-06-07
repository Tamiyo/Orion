#include "yuzu/Hir/Ops/BuiltinOps.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/Trait.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeCoercion.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FormatVariadic.h>

namespace yuzu::hir {
namespace {

/// "operator can't be applied to these operands" diagnostic, anchored across
/// both operands. Returns the error type so a resolve can tail-call it.
const Type *unsupportedOperands(llvm::ArrayRef<const Expr *> args,
                                HirContext &ctx, llvm::StringRef symbol) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  // Resolve holes so an unpinned literal reports its default (`int64`),
  // not the internal `<infer>`.
  const auto *lhs = types.resolveType(types.typeOf(args[0]));
  const auto *rhs = types.resolveType(types.typeOf(args[1]));
  ctx.error(args, llvm::formatv("binary operator `{0}` cannot be applied to "
                                "`{1}` and `{2}`",
                                symbol, asString(lhs->getKind()),
                                asString(rhs->getKind()))
                      .str())
      .emit();

  return typeFactory.getErrorType();
}

/// Unary counterpart of `unsupportedOperands`.
const Type *unsupportedUnaryOperand(llvm::ArrayRef<const Expr *> args,
                                    HirContext &ctx, llvm::StringRef symbol) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  const auto *operand = types.resolveType(types.typeOf(args[0]));
  ctx.error(args,
            llvm::formatv("unary operator `{0}` cannot be applied to `{1}`",
                          symbol, asString(operand->getKind()))
                .str())
      .emit();
  return typeFactory.getErrorType();
}

/// Shared binary resolve: coerce operands to a common type, then consult the
/// trait table for whether that type implements `trait` and what it yields.
/// A null table result means "same as the operand" (so `5 + 5` keeps its open
/// hole); a fixed result (e.g. `bool` for comparisons) is returned as-is.
const Type *resolveBinaryTrait(llvm::ArrayRef<const Expr *> args,
                               HirContext &ctx, Trait trait,
                               llvm::StringRef symbol) {
  auto &types = ctx.getTypeContext();

  const Type *common = coerceTypes(args[0], args[1], ctx);
  if (common == nullptr) {
    return unsupportedOperands(args, ctx, symbol);
  }

  // A hole looks up under its default type, but the op still yields the hole.
  const auto result =
      types.getTraitTable().lookup(trait, types.resolveType(common));
  if (!result) {
    return unsupportedOperands(args, ctx, symbol);
  }
  return *result != nullptr ? *result : common;
}

/// Shared unary resolve: consult the trait table for the operand's type.
const Type *resolveUnaryTrait(llvm::ArrayRef<const Expr *> args,
                              HirContext &ctx, Trait trait,
                              llvm::StringRef symbol) {
  auto &types = ctx.getTypeContext();
  const Type *operand = types.typeOf(args[0]);

  const auto result =
      types.getTraitTable().lookup(trait, types.resolveType(operand));
  if (!result) {
    return unsupportedUnaryOperand(args, ctx, symbol);
  }
  return *result != nullptr ? *result : operand;
}

/// Pin literal holes to a concrete type before an op that inspects integer
/// kinds directly (the shifts).
void resolveOperandTypes(llvm::ArrayRef<const Expr *> args, HirContext &ctx) {
  for (const Expr *arg : args) {
    ctx.getTypeContext().concretize(arg);
  }
}

} // namespace

//===----------------------------------------------------------------------===//
// Trait-backed operators — coerce, then look the operation up in the trait
// table. Per-type behavior lives in the table, not here.
//===----------------------------------------------------------------------===//

const Type *AddOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Add, "+");
}

const Type *SubOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Sub, "-");
}

const Type *MulOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Mul, "*");
}

const Type *DivOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Div, "/");
}

const Type *PowOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Pow, "**");
}

const Type *AndOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::And, "and");
}

const Type *OrOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Or, "or");
}

const Type *EqOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Eq, "==");
}

const Type *NeqOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Neq, "!=");
}

const Type *LtOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Lt, "<");
}

const Type *LteOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Lte, "<=");
}

const Type *GtOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Gt, ">");
}

const Type *GteOp::resolve(llvm::ArrayRef<const Expr *> args,
                           HirContext &ctx) const {
  return resolveBinaryTrait(args, ctx, Trait::Gte, ">=");
}

const Type *UnaryPosOp::resolve(llvm::ArrayRef<const Expr *> args,
                                HirContext &ctx) const {
  return resolveUnaryTrait(args, ctx, Trait::Pos, "+");
}

const Type *UnaryNegOp::resolve(llvm::ArrayRef<const Expr *> args,
                                HirContext &ctx) const {
  return resolveUnaryTrait(args, ctx, Trait::Neg, "-");
}

const Type *UnaryNotOp::resolve(llvm::ArrayRef<const Expr *> args,
                                HirContext &ctx) const {
  return resolveUnaryTrait(args, ctx, Trait::Not, "not");
}

//===----------------------------------------------------------------------===//
// Membership — no container types exist yet, so the only definable form is
// substring containment (`str in str`). Not yet modeled as a trait.
//===----------------------------------------------------------------------===//

const Type *InOp::resolve(llvm::ArrayRef<const Expr *> args,
                          HirContext &ctx) const {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  if (types.typeOf(args[0])->getKind() == TypeKind::Str &&
      types.typeOf(args[1])->getKind() == TypeKind::Str) {
    return typeFactory.getBoolType();
  }
  return unsupportedOperands(args, ctx, "in");
}

const Type *NotInOp::resolve(llvm::ArrayRef<const Expr *> args,
                             HirContext &ctx) const {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  if (types.typeOf(args[0])->getKind() == TypeKind::Str &&
      types.typeOf(args[1])->getKind() == TypeKind::Str) {
    return typeFactory.getBoolType();
  }
  return unsupportedOperands(args, ctx, "not in");
}

//===----------------------------------------------------------------------===//
// Bit shifts — both operands must be integers; the result is the left
// operand's type. The shift count isn't coerced against the value, so this
// stays hand-written rather than trait-backed.
//===----------------------------------------------------------------------===//

const Type *ShiftLeftOp::resolve(llvm::ArrayRef<const Expr *> args,
                                 HirContext &ctx) const {
  resolveOperandTypes(args, ctx);
  auto &types = ctx.getTypeContext();
  if (types.typeOf(args[0])->isInt() && types.typeOf(args[1])->isInt()) {
    return types.typeOf(args[0]);
  }
  return unsupportedOperands(args, ctx, "<<");
}

const Type *ShiftRightOp::resolve(llvm::ArrayRef<const Expr *> args,
                                  HirContext &ctx) const {
  resolveOperandTypes(args, ctx);
  auto &types = ctx.getTypeContext();
  if (types.typeOf(args[0])->isInt() && types.typeOf(args[1])->isInt()) {
    return types.typeOf(args[0]);
  }
  return unsupportedOperands(args, ctx, ">>");
}

} // namespace yuzu::hir