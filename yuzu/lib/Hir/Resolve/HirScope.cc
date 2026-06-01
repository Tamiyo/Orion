#include "yuzu/Hir/Resolve/HirScope.h"

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/Resolve/HirSymbolTable.h"
#include "yuzu/Util/U32StringExtensions.h" // IWYU pragma: keep
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/FormatVariadic.h>

namespace yuzu::hir {
HirScopeGuard::~HirScopeGuard() {
  if (table != nullptr) {
    table->popScope();
  }
}

void HirScope::bind(const Ident *ident, Binding decl) {
  if (bindings.contains(ident->getName())) {
    ctx.error(ident, llvm::formatv("`{0}` is already bound",
                                   util::toUtf8(ident->getName()))
                         .str())
        .emit();
    return;
  }

  bindings[ident->getName()] = decl;
}
} // namespace yuzu::hir