#ifndef YUZU_HIR_RESOLVE_HIRSCOPE_H
#define YUZU_HIR_RESOLVE_HIRSCOPE_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Util/StringInterner.h" // IWYU pragma: keep — DenseMapInfo<u32string_view>

#include <llvm/ADT/DenseMap.h>

#include <cstdint>
#include <string_view>

namespace yuzu::hir {
class HirSymbolTable;

enum class [[nodiscard]] HirScopeKind : uint8_t { Block };

/// A lexical scope's bindings + its back-pointer to the owning symbol
/// table.
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

  void bind(const Ident *ident, const LetStmt *decl) {
    bindings[ident->getName()] = decl;
  }

  /// Declaration bound to `ident`'s name, or null if this scope has no
  /// entry. `find` rather than `operator[]` so a miss doesn't insert.
  const LetStmt *lookup(const Ident *ident) const {
    const auto it = bindings.find(ident->getName());
    if (it == bindings.end()) {
      return nullptr;
    }
    return it->second;
  }

private:
  llvm::DenseMap<std::u32string_view, const LetStmt *> bindings;
  HirSymbolTable &symbolTable;
  HirScopeKind kind;
};
} // namespace yuzu::hir

#endif
