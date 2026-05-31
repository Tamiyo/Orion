#ifndef YUZU_HIR_TYPES_TYPECONCRETIZER_H
#define YUZU_HIR_TYPES_TYPECONCRETIZER_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/HirVisitor.h"

namespace yuzu::hir {
/// Walk that resolves every node's type — inference holes follow their fills
/// or default to a concrete type. Runs after `TypeInferrer`; type-free nodes
/// are left untouched.
class TypeConcretizer final : public HirVisitor<TypeConcretizer> {
public:
  explicit TypeConcretizer(HirContext &ctx) : ctx(ctx) {}

  void visit(const HirNode *node) {
    HirVisitor::visit(node);
    ctx.getTypeContext().concretize(node);
  }

private:
  HirContext &ctx;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPECONCRETIZER_H