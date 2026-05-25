#ifndef YUZU_HIR_RESOLVE_HIRSCOPE_H
#define YUZU_HIR_RESOLVE_HIRSCOPE_H

#include "yuzu/Hir/Hir.h"

#include <cstdint>
#include <map>
#include <string>

namespace yuzu::hir {
class HirSymbolTable;

enum class [[nodiscard]] HirScopeKind : uint8_t { Block };

/// A lexical scope's bindings + its back-pointer to the owning symbol
/// table. Held by `HirSymbolTable` in a `std::deque` so addresses
/// stay stable across push/pop. Keyed by name string (not by `Ident *`
/// identity) because a binding-site `Ident` and a use-site `Ident`
/// are independently lowered — they share a name, not a pointer.
class [[nodiscard]] HirScope {
public:
  explicit HirScope(HirSymbolTable &symbolTable, HirScopeKind kind)
      : symbolTable(symbolTable), kind(kind) {}

  ~HirScope();

  HirScope(const HirScope &) = delete;
  HirScope &operator=(const HirScope &) = delete;
  HirScope(HirScope &&) = delete;
  HirScope &operator=(HirScope &&) = delete;

  HirScopeKind getKind() const { return kind; }

  void bind(const Ident *ident, const Expr *expr) {
    bindings[ident->getName()] = expr;
  }

  /// Returns the bound defining node for `ident`'s name, or nullptr
  /// if this scope has no entry. Uses `find` rather than `operator[]`
  /// so a miss doesn't insert a stray default entry.
  const Expr *lookup(const Ident *ident) const {
    const auto it = bindings.find(ident->getName());
    if (it == bindings.end()) {
      return nullptr;
    }
    return it->second;
  }

private:
  // `std::map` rather than `llvm::DenseMap` because `DenseMap` needs a
  // `DenseMapInfo<std::u32string>` specialization that doesn't ship
  // with LLVM. The scope map is small and queried per identifier; the
  // O(log n) cost is fine until interned `Symbol *` keys land.
  std::map<std::u32string, const Expr *> bindings;
  HirSymbolTable &symbolTable;
  HirScopeKind kind;
};
} // namespace yuzu::hir

#endif
