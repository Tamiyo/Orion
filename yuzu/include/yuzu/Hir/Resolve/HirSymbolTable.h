#ifndef YUZU_HIR_RESOLVE_HIRSYMBOLTABLE_H
#define YUZU_HIR_RESOLVE_HIRSYMBOLTABLE_H

#include "yuzu/Hir/Resolve/HirScope.h"

#include <deque>
#include <optional>

namespace yuzu::hir {
class [[nodiscard]] HirSymbolTable {
public:
  HirSymbolTable(const HirSymbolTable &) = delete;
  HirSymbolTable &operator=(const HirSymbolTable &) = delete;
  HirSymbolTable(HirSymbolTable &&) = delete;
  HirSymbolTable &operator=(HirSymbolTable &&) = delete;

  explicit HirSymbolTable(HirContext &ctx);

  /// Push a fresh innermost scope and return a guard that pops it when the
  /// guard leaves C++ scope. Bind into / look up the new scope until then.
  HirScopeGuard pushScope(HirScopeKind kind);

  /// Bind `ident`'s name to its declaration in the current (innermost)
  /// scope. Re-binding the same name in the same scope overwrites —
  /// shadowing across scopes is the lookup loop's job.
  void bind(const Ident *ident, HirScope::Binding decl) {
    scopes.back().bind(ident, decl);
  }

  /// Walk from the innermost scope outward, returning the first
  /// declaration bound to `ident`'s name. `nullopt` if unbound.
  HirScope::LookupResult lookup(const Ident *ident) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      if (const auto result = it->lookup(ident)) {
        return result;
      }
    }
    return std::nullopt;
  }

private:
  // Only `HirScopeGuard` pops, on its own destruction — keeping the pop
  // off the scope's own destructor avoids mutating `scopes` mid-teardown.
  friend class HirScopeGuard;
  void popScope() { scopes.pop_back(); }

  std::deque<HirScope> scopes;
  HirContext &ctx;
};
} // namespace yuzu::hir

#endif
