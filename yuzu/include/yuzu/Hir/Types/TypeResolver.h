#ifndef YUZU_HIR_TYPES_TYPERESOLVER_H
#define YUZU_HIR_TYPES_TYPERESOLVER_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"

namespace yuzu::hir {
/// Semantic pass over a lowered HIR tree: a `TypeInferrer` walk types every
/// node, then a `TypeConcretizer` walk resolves the inference holes it left.
class TypeResolver final {
public:
  explicit TypeResolver(HirContext &ctx) : ctx(ctx) {}

  TypeResolver() = delete;

  void resolve(const Root *root);

private:
  HirContext &ctx;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPERESOLVER_H
