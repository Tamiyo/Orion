#include "yuzu/Hir/Resolve/HirSymbolTable.h"

#include "yuzu/Hir/Resolve/HirScope.h"

namespace yuzu::hir {
HirSymbolTable::HirSymbolTable() {
  scopes.emplace_back(*this, HirScopeKind::Block);
}

// Flip the guard *before* the implicit member-destruction sequence
// runs. After this line, the deque's dtor fires and destroys each
// `HirScope`; each ~HirScope calls back into `popScope`, which
// short-circuits on `destructing` instead of mutating the deque
// mid-destruction.
HirSymbolTable::~HirSymbolTable() { destructing = true; }

HirScope &HirSymbolTable::pushScope(HirScopeKind kind) {
  scopes.emplace_back(*this, kind);
  return scopes.back();
}

void HirSymbolTable::popScope() {
  if (destructing) {
    return;
  }
  scopes.pop_back();
}
} // namespace yuzu::hir