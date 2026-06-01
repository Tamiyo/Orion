#include "yuzu/Hir/Resolve/HirSymbolTable.h"

#include "yuzu/Hir/Resolve/HirScope.h"

namespace yuzu::hir {
HirSymbolTable::HirSymbolTable(HirContext &ctx) : ctx(ctx) {
  // The outermost (module) scope. It is never popped — no guard owns it —
  // so it lives for the table's whole lifetime.
  scopes.emplace_back(ctx, HirScopeKind::Block);
}

HirScopeGuard HirSymbolTable::pushScope(HirScopeKind kind) {
  scopes.emplace_back(ctx, kind);
  return HirScopeGuard(*this);
}
} // namespace yuzu::hir