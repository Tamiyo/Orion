#ifndef YUZU_HIR_OP_BUILTINOP_H
#define YUZU_HIR_OP_BUILTINOP_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/Op.h"

#include <llvm/ADT/ArrayRef.h>

#include <string_view>

namespace yuzu::hir {

#define YUZU_HIR_SINGLETON_OP(ClassName)                                       \
  class ClassName##Op final : public Op {                                      \
  public:                                                                      \
    static constexpr std::string_view Name = #ClassName;                       \
                                                                               \
    ClassName##Op() : Op(OpKind::Builtin) {}                                   \
                                                                               \
    static const ClassName##Op *get() {                                        \
      static const ClassName##Op instance;                                     \
      return &instance;                                                        \
    }                                                                          \
                                                                               \
    [[nodiscard]] std::string_view getName() const override { return Name; }   \
                                                                               \
    const types::Type *resolve(llvm::ArrayRef<const Expr *> args,              \
                               HirContext &ctx) const override;                \
  };

YUZU_HIR_SINGLETON_OP(Add)
YUZU_HIR_SINGLETON_OP(And)
YUZU_HIR_SINGLETON_OP(Sub)
YUZU_HIR_SINGLETON_OP(Mul)
YUZU_HIR_SINGLETON_OP(Div)
YUZU_HIR_SINGLETON_OP(Or)
YUZU_HIR_SINGLETON_OP(In)
YUZU_HIR_SINGLETON_OP(NotIn)
YUZU_HIR_SINGLETON_OP(Pow)
YUZU_HIR_SINGLETON_OP(Eq)
YUZU_HIR_SINGLETON_OP(Neq)
YUZU_HIR_SINGLETON_OP(Lt)
YUZU_HIR_SINGLETON_OP(Lte)
YUZU_HIR_SINGLETON_OP(Gt)
YUZU_HIR_SINGLETON_OP(Gte)
YUZU_HIR_SINGLETON_OP(ShiftLeft)
YUZU_HIR_SINGLETON_OP(ShiftRight)

YUZU_HIR_SINGLETON_OP(UnaryPos)
YUZU_HIR_SINGLETON_OP(UnaryNeg)
YUZU_HIR_SINGLETON_OP(UnaryNot)

} // namespace yuzu::hir

#endif // YUZU_HIR_OP_BUILTINOP_H
