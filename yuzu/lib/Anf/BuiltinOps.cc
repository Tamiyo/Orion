#include "yuzu/Anf/Ops/BuiltinOps.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Anf/Reduction/AnfConstants.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/Twine.h>

#include <cmath>
#include <cstdint>

// Each op's `fold` takes the operands it was called with — already resolved to
// their defining constants by the reducer — and branches on what the left and
// right operands actually are (`isInt`, `isFloat`, ...). An unhandled
// combination, or a non-constant operand, returns null: "can't fold". Adding a
// case — string concatenation for `Add`, say — is one more branch.
//
// `Eq`, `Lt`, and `Gt` carry the per-type branches; `Neq`, `Lte`, and `Gte`
// are their negations.

namespace yuzu::anf {

using types::Type;

namespace {

// Negate a folded boolean (null propagates as "didn't fold").
const Constant *negate(const Constant *result, AnfContext &ctx,
                       const Type *type) {
  if (result == nullptr) {
    return nullptr;
  }
  return ctx.getBuilder().makeBoolConst(!asBool(result), type);
}

// Warn that folding `operation` would overflow `int64`, anchored at `anchor`,
// and decline to fold (so the wrapped value isn't silently baked in).
const Constant *emitOverflowWarning(AnfContext &ctx, const Atom *anchor,
                                    llvm::StringRef operation) {
  ctx.warning(anchor,
              llvm::Twine("integer overflow folding constant ") + operation)
      .emit();
  return nullptr;
}

} // namespace

const Constant *AddOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                            const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    int64_t result = 0;
    if (__builtin_add_overflow(asInt(left), asInt(right), &result)) {
      return emitOverflowWarning(ctx, left, "addition");
    }
    return ctx.getBuilder().makeIntConst(result, type);
  }
  if (isFloat(left) && isFloat(right)) {
    return ctx.getBuilder().makeFloatConst(asFloat(left) + asFloat(right),
                                           type);
  }
  return nullptr;
}

const Constant *SubOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                            const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    int64_t result = 0;
    if (__builtin_sub_overflow(asInt(left), asInt(right), &result)) {
      return emitOverflowWarning(ctx, left, "subtraction");
    }
    return ctx.getBuilder().makeIntConst(result, type);
  }
  if (isFloat(left) && isFloat(right)) {
    return ctx.getBuilder().makeFloatConst(asFloat(left) - asFloat(right),
                                           type);
  }
  return nullptr;
}

const Constant *MulOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                            const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    int64_t result = 0;
    if (__builtin_mul_overflow(asInt(left), asInt(right), &result)) {
      return emitOverflowWarning(ctx, left, "multiplication");
    }
    return ctx.getBuilder().makeIntConst(result, type);
  }
  if (isFloat(left) && isFloat(right)) {
    return ctx.getBuilder().makeFloatConst(asFloat(left) * asFloat(right),
                                           type);
  }
  return nullptr;
}

const Constant *DivOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                            const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    // Integer division by zero traps; leave it for the runtime to report.
    if (asInt(right) == 0) {
      return nullptr;
    }
    return ctx.getBuilder().makeIntConst(asInt(left) / asInt(right), type);
  }
  if (isFloat(left) && isFloat(right)) {
    return ctx.getBuilder().makeFloatConst(asFloat(left) / asFloat(right),
                                           type);
  }
  return nullptr;
}

const Constant *PowOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                            const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    // A negative integer exponent isn't an integer; don't fold.
    if (asInt(right) < 0) {
      return nullptr;
    }
    int64_t result = 1;
    for (int64_t i = 0; i < asInt(right); ++i) {
      if (__builtin_mul_overflow(result, asInt(left), &result)) {
        return emitOverflowWarning(ctx, left, "exponentiation");
      }
    }
    return ctx.getBuilder().makeIntConst(result, type);
  }
  if (isFloat(left) && isFloat(right)) {
    return ctx.getBuilder().makeFloatConst(
        std::pow(asFloat(left), asFloat(right)), type);
  }
  return nullptr;
}

const Constant *EqOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                           const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    return ctx.getBuilder().makeBoolConst(asInt(left) == asInt(right), type);
  }
  if (isFloat(left) && isFloat(right)) {
    return ctx.getBuilder().makeBoolConst(asFloat(left) == asFloat(right),
                                          type);
  }
  if (isBool(left) && isBool(right)) {
    return ctx.getBuilder().makeBoolConst(asBool(left) == asBool(right), type);
  }
  return nullptr;
}

const Constant *LtOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                           const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    return ctx.getBuilder().makeBoolConst(asInt(left) < asInt(right), type);
  }
  if (isFloat(left) && isFloat(right)) {
    return ctx.getBuilder().makeBoolConst(asFloat(left) < asFloat(right), type);
  }
  return nullptr;
}

const Constant *GtOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                           const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    return ctx.getBuilder().makeBoolConst(asInt(left) > asInt(right), type);
  }
  if (isFloat(left) && isFloat(right)) {
    return ctx.getBuilder().makeBoolConst(asFloat(left) > asFloat(right), type);
  }
  return nullptr;
}

// a != b  <=>  !(a == b)
const Constant *NeqOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                            const Type *type) const {
  return negate(EqOp::get()->fold(args, ctx, type), ctx, type);
}

// a <= b  <=>  !(a > b)
const Constant *LteOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                            const Type *type) const {
  return negate(GtOp::get()->fold(args, ctx, type), ctx, type);
}

// a >= b  <=>  !(a < b)
const Constant *GteOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                            const Type *type) const {
  return negate(LtOp::get()->fold(args, ctx, type), ctx, type);
}

const Constant *AndOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                            const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isBool(left) && isBool(right)) {
    return ctx.getBuilder().makeBoolConst(asBool(left) && asBool(right), type);
  }
  return nullptr;
}

const Constant *OrOp::fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,
                           const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isBool(left) && isBool(right)) {
    return ctx.getBuilder().makeBoolConst(asBool(left) || asBool(right), type);
  }
  return nullptr;
}

const Constant *ShiftLeftOp::fold(llvm::ArrayRef<const Atom *> args,
                                  AnfContext &ctx, const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    // A shift amount outside [0, 63] is undefined; don't fold it.
    if (asInt(right) < 0 || asInt(right) >= 64) {
      return nullptr;
    }
    const int64_t value = asInt(left);
    const int64_t shift = asInt(right);
    if (value > (INT64_MAX >> shift) || value < (INT64_MIN >> shift)) {
      return emitOverflowWarning(ctx, left, "left shift");
    }
    return ctx.getBuilder().makeIntConst(value << shift, type);
  }
  return nullptr;
}

const Constant *ShiftRightOp::fold(llvm::ArrayRef<const Atom *> args,
                                   AnfContext &ctx, const Type *type) const {
  if (args.size() != 2) {
    return nullptr;
  }
  const Atom *left = args[0];
  const Atom *right = args[1];

  if (isInt(left) && isInt(right)) {
    if (asInt(right) < 0 || asInt(right) >= 64) {
      return nullptr;
    }
    return ctx.getBuilder().makeIntConst(asInt(left) >> asInt(right), type);
  }
  return nullptr;
}

const Constant *UnaryPosOp::fold(llvm::ArrayRef<const Atom *> args,
                                 AnfContext &, const Type *) const {
  if (args.size() != 1) {
    return nullptr;
  }
  // `+x` is the identity on a numeric constant.
  if (isInt(args[0]) || isFloat(args[0])) {
    return Constant::cast(args[0]);
  }
  return nullptr;
}

const Constant *UnaryNegOp::fold(llvm::ArrayRef<const Atom *> args,
                                 AnfContext &ctx, const Type *type) const {
  if (args.size() != 1) {
    return nullptr;
  }
  if (isInt(args[0])) {
    int64_t result = 0;
    if (__builtin_sub_overflow(int64_t{0}, asInt(args[0]), &result)) {
      return emitOverflowWarning(ctx, args[0], "negation");
    }
    return ctx.getBuilder().makeIntConst(result, type);
  }
  if (isFloat(args[0])) {
    return ctx.getBuilder().makeFloatConst(-asFloat(args[0]), type);
  }
  return nullptr;
}

const Constant *UnaryNotOp::fold(llvm::ArrayRef<const Atom *> args,
                                 AnfContext &ctx, const Type *type) const {
  if (args.size() != 1) {
    return nullptr;
  }
  if (isBool(args[0])) {
    return ctx.getBuilder().makeBoolConst(!asBool(args[0]), type);
  }
  return nullptr;
}

// `In` / `NotIn` fold once the collection operand is a constant — which needs a
// collection-shaped `Constant` atom we don't have yet. Until then, no fold.
const Constant *InOp::fold(llvm::ArrayRef<const Atom *>, AnfContext &,
                           const Type *) const {
  return nullptr;
}

const Constant *NotInOp::fold(llvm::ArrayRef<const Atom *>, AnfContext &,
                              const Type *) const {
  return nullptr;
}

} // namespace yuzu::anf
