#include "yuzu/Hir/Types/TypeResolver.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/Types/TypeConcretizer.h"
#include "yuzu/Hir/Types/TypeInferrer.h"

namespace yuzu::hir {

void TypeResolver::resolve(const Root *root) {
  TypeInferrer(ctx).visit(root);
  TypeConcretizer(ctx).visit(root);
}

} // namespace yuzu::hir