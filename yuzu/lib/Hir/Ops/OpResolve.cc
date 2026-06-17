#include "yuzu/Hir/Ops/OpResolve.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/Trait.h"
#include "yuzu/Hir/Types/TypeCoercion.h"
#include "yuzu/Types/Type.h"
#include "yuzu/Util/ErrorHandling.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FormatVariadic.h>

namespace yuzu::hir {
using namespace yuzu::types;
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
                               HirContext &ctx, std::u32string_view trait,
                               llvm::StringRef symbol) {
  auto &types = ctx.getTypeContext();

  const Type *common = coerceTypes(args[0], args[1], ctx);
  if (common == nullptr) {
    return unsupportedOperands(args, ctx, symbol);
  }

  // A hole looks up under its default type, but the op still yields the hole.
  const auto result =
      types.getTraitRegistry().lookupImpl(trait, types.resolveType(common));
  if (!result) {
    return unsupportedOperands(args, ctx, symbol);
  }
  return *result != nullptr ? *result : common;
}

/// Shared unary resolve: consult the trait registry for the operand's type.
const Type *resolveUnaryTrait(llvm::ArrayRef<const Expr *> args,
                              HirContext &ctx, std::u32string_view trait,
                              llvm::StringRef symbol) {
  auto &types = ctx.getTypeContext();
  const Type *operand = types.typeOf(args[0]);

  const auto result =
      types.getTraitRegistry().lookupImpl(trait, types.resolveType(operand));
  if (!result) {
    return unsupportedUnaryOperand(args, ctx, symbol);
  }
  return *result != nullptr ? *result : operand;
}

/// Membership (`in` / `not in`) — no container types exist yet, so the only
/// definable form is substring containment (`str in str`). Not a trait.
const Type *resolveMembership(llvm::ArrayRef<const Expr *> args,
                              HirContext &ctx, llvm::StringRef symbol) {
  auto &types = ctx.getTypeContext();
  auto &typeFactory = types.getTypeFactory();

  if (types.typeOf(args[0])->getKind() == TypeKind::Str &&
      types.typeOf(args[1])->getKind() == TypeKind::Str) {
    return typeFactory.getBoolType();
  }
  return unsupportedOperands(args, ctx, symbol);
}

/// Bit shifts — both operands must be integers; the result is the left
/// operand's type. The shift count isn't coerced against the value, so this
/// stays hand-written rather than trait-backed.
const Type *resolveShift(llvm::ArrayRef<const Expr *> args, HirContext &ctx,
                         llvm::StringRef symbol) {
  // Pin literal holes to a concrete type before inspecting integer kinds.
  for (const Expr *arg : args) {
    ctx.getTypeContext().concretize(arg);
  }
  auto &types = ctx.getTypeContext();
  if (types.typeOf(args[0])->isInt() && types.typeOf(args[1])->isInt()) {
    return types.typeOf(args[0]);
  }
  return unsupportedOperands(args, ctx, symbol);
}

} // namespace

const Type *resolve(BuiltinOp op, llvm::ArrayRef<const Expr *> args,
                    HirContext &ctx) {
  switch (op) {
  case BuiltinOp::Add:
    return resolveBinaryTrait(args, ctx, U"Add", "+");
  case BuiltinOp::Sub:
    return resolveBinaryTrait(args, ctx, U"Sub", "-");
  case BuiltinOp::Mul:
    return resolveBinaryTrait(args, ctx, U"Mul", "*");
  case BuiltinOp::Div:
    return resolveBinaryTrait(args, ctx, U"Div", "/");
  case BuiltinOp::Pow:
    return resolveBinaryTrait(args, ctx, U"Pow", "**");
  case BuiltinOp::And:
    return resolveBinaryTrait(args, ctx, U"And", "and");
  case BuiltinOp::Or:
    return resolveBinaryTrait(args, ctx, U"Or", "or");
  case BuiltinOp::Eq:
    return resolveBinaryTrait(args, ctx, U"Eq", "==");
  case BuiltinOp::Neq:
    return resolveBinaryTrait(args, ctx, U"Neq", "!=");
  case BuiltinOp::Lt:
    return resolveBinaryTrait(args, ctx, U"Lt", "<");
  case BuiltinOp::Lte:
    return resolveBinaryTrait(args, ctx, U"Lte", "<=");
  case BuiltinOp::Gt:
    return resolveBinaryTrait(args, ctx, U"Gt", ">");
  case BuiltinOp::Gte:
    return resolveBinaryTrait(args, ctx, U"Gte", ">=");
  case BuiltinOp::In:
    return resolveMembership(args, ctx, "in");
  case BuiltinOp::NotIn:
    return resolveMembership(args, ctx, "not in");
  case BuiltinOp::ShiftLeft:
    return resolveShift(args, ctx, "<<");
  case BuiltinOp::ShiftRight:
    return resolveShift(args, ctx, ">>");
  case BuiltinOp::UnaryPos:
    return resolveUnaryTrait(args, ctx, U"Pos", "+");
  case BuiltinOp::UnaryNeg:
    return resolveUnaryTrait(args, ctx, U"Neg", "-");
  case BuiltinOp::UnaryNot:
    return resolveUnaryTrait(args, ctx, U"Not", "not");
  }
  util::yuzu_unreachable("unknown BuiltinOp in resolve");
}

} // namespace yuzu::hir
