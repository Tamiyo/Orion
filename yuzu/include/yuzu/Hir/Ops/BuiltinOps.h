#ifndef YUZU_HIR_OP_BUILTINOP_H
#define YUZU_HIR_OP_BUILTINOP_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Ops/Op.h"

#include <llvm/ADT/ArrayRef.h>

#include <string_view>

namespace yuzu::hir {

#define YUZU_SINGLETON_OP(ClassName)                                           \
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
    const Type *resolve(llvm::ArrayRef<const Expr *> args,                     \
                        HirContext &ctx) const override;                       \
  };

YUZU_SINGLETON_OP(Add)
YUZU_SINGLETON_OP(Sub)
YUZU_SINGLETON_OP(Mul)
YUZU_SINGLETON_OP(Div)

} // namespace yuzu::hir

#endif // YUZU_HIR_OP_BUILTINOP_H
