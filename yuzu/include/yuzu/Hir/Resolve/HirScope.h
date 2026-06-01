#ifndef YUZU_HIR_RESOLVE_HIRSCOPE_H
#define YUZU_HIR_RESOLVE_HIRSCOPE_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Util/U32StringExtensions.h" // IWYU pragma: keep

#include <llvm/ADT/DenseMap.h>

#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>

namespace yuzu::hir {
class HirContext;
class HirSymbolTable;

enum class [[nodiscard]] HirScopeKind : uint8_t { Fn, Block };

/// A lexical scope's bindings. Owned by `HirSymbolTable`'s scope stack;
/// lifetime is managed by `HirScopeGuard`, not by this object's destructor
/// (a scope must not pop the stack it lives in).
class [[nodiscard]] HirScope {
public:
  /// The declaring nodes a name may bind to. The variant is the allowlist —
  /// only these kinds can enter the table, and a lookup is provably one of
  /// them (no stray `HirNode`). Every binding's type lives in the type side
  /// table, so a consumer reads it uniformly via `typeOf(decl)` regardless
  /// of which arm it is.
  using Binding = std::variant<const LetStmt *, const Param *, const FnStmt *>;
  using LookupResult = std::optional<Binding>;

  explicit HirScope(HirContext &ctx, HirScopeKind kind) : ctx(ctx), kind(kind) {}

  HirScope(const HirScope &) = delete;
  HirScope &operator=(const HirScope &) = delete;
  HirScope(HirScope &&) = delete;
  HirScope &operator=(HirScope &&) = delete;

  HirScopeKind getKind() const { return kind; }

  void bind(const Ident *ident, Binding decl);

  /// Declaration bound to `ident`'s name, or `nullopt` if this scope has no
  /// entry. `find` rather than `operator[]` so a miss doesn't insert.
  LookupResult lookup(const Ident *ident) const {
    const auto it = bindings.find(ident->getName());
    if (it == bindings.end()) {
      return std::nullopt;
    }
    return it->second;
  }

private:
  llvm::DenseMap<std::u32string_view, Binding> bindings;
  HirContext &ctx;
  HirScopeKind kind;
};

/// RAII handle for a pushed scope: pops it off the symbol table's stack when
/// the guard leaves C++ scope. Move-only; created by `pushScope`. Keeping the
/// pop here (rather than in `~HirScope`) avoids a scope mutating the deque it
/// is stored in mid-destruction.
class [[nodiscard]] HirScopeGuard {
public:
  explicit HirScopeGuard(HirSymbolTable &table) : table(&table) {}

  HirScopeGuard(const HirScopeGuard &) = delete;
  HirScopeGuard &operator=(const HirScopeGuard &) = delete;
  HirScopeGuard(HirScopeGuard &&other) noexcept : table(other.table) {
    other.table = nullptr;
  }
  HirScopeGuard &operator=(HirScopeGuard &&) = delete;

  ~HirScopeGuard();

private:
  HirSymbolTable *table;
};
} // namespace yuzu::hir

#endif
