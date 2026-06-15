#ifndef YUZU_ANF_OPS_BUILTINOPS_H
#define YUZU_ANF_OPS_BUILTINOPS_H

#include "yuzu/Anf/Ops/Op.h"

#include <string_view>

namespace yuzu::anf {

// Mirrors `hir::BuiltinOps`, minus `resolve` — ANF is already typed. Each is a
// stateless singleton, compared by pointer, and carries its own constant-fold
// rule (defined in `BuiltinOps.cc`).
#define YUZU_ANF_SINGLETON_OP(ClassName)                                       \
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
    [[nodiscard]] const Constant *                                             \
    fold(llvm::ArrayRef<const Atom *> args, AnfContext &ctx,                   \
         const types::Type *type) const override;                              \
  };

YUZU_ANF_SINGLETON_OP(Add)
YUZU_ANF_SINGLETON_OP(And)
YUZU_ANF_SINGLETON_OP(Sub)
YUZU_ANF_SINGLETON_OP(Mul)
YUZU_ANF_SINGLETON_OP(Div)
YUZU_ANF_SINGLETON_OP(Or)
YUZU_ANF_SINGLETON_OP(In)
YUZU_ANF_SINGLETON_OP(NotIn)
YUZU_ANF_SINGLETON_OP(Pow)
YUZU_ANF_SINGLETON_OP(Eq)
YUZU_ANF_SINGLETON_OP(Neq)
YUZU_ANF_SINGLETON_OP(Lt)
YUZU_ANF_SINGLETON_OP(Lte)
YUZU_ANF_SINGLETON_OP(Gt)
YUZU_ANF_SINGLETON_OP(Gte)
YUZU_ANF_SINGLETON_OP(ShiftLeft)
YUZU_ANF_SINGLETON_OP(ShiftRight)

YUZU_ANF_SINGLETON_OP(UnaryPos)
YUZU_ANF_SINGLETON_OP(UnaryNeg)
YUZU_ANF_SINGLETON_OP(UnaryNot)

#undef YUZU_ANF_SINGLETON_OP

} // namespace yuzu::anf

#endif // YUZU_ANF_OPS_BUILTINOPS_H
