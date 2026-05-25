#ifndef YUZU_HIR_RESOLVE_HIRSYMBOLTABLE_H
#define YUZU_HIR_RESOLVE_HIRSYMBOLTABLE_H

#include "yuzu/Hir/Resolve/HirScope.h"

#include <deque>

namespace yuzu::hir {
class [[nodiscard]] HirSymbolTable {
public:
  HirSymbolTable(const HirSymbolTable &) = delete;
  HirSymbolTable &operator=(const HirSymbolTable &) = delete;
  HirSymbolTable(HirSymbolTable &&) = delete;
  HirSymbolTable &operator=(HirSymbolTable &&) = delete;

  explicit HirSymbolTable();
  ~HirSymbolTable();

  HirScope &pushScope(HirScopeKind kind);

  /// Bind `ident`'s name to `def` in the current (innermost) scope.
  /// Re-binding the same name in the same scope overwrites — shadowing
  /// across scopes is the lookup loop's job.
  void bind(const Ident *ident, const Expr *expr) {
    scopes.back().bind(ident, expr);
  }

  /// Walk from the innermost scope outward, returning the first
  /// binding for `ident`'s name. `nullptr` if the name is unbound
  /// anywhere along the chain.
  const Expr *lookup(const Ident *ident) const {
    for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
      if (const Expr *expr = it->lookup(ident)) {
        return expr;
      }
    }
    return nullptr;
  }

private:
  // `HirScope::~HirScope` calls `popScope` to drop itself off the
  // stack; friending keeps that the only path that pops the table.
  friend class HirScope;
  void popScope();

  // `HirScope` is stored *inside* `scopes`, so the natural teardown
  // path is `~HirSymbolTable` → deque dtor → element dtors →
  // `popScope` → `scopes.pop_back()` *during* the deque's own
  // destruction (UB / segfault). The dtor flips this before the
  // deque dies so `popScope` short-circuits during teardown. Long
  // term, separating the scope *data* (owned by the deque) from a
  // stack-allocated *guard* (which calls `popScope` on its own
  // destruction) would eliminate the need for this entirely.
  bool destructing = false;

  std::deque<HirScope> scopes;
};
} // namespace yuzu::hir

#endif
