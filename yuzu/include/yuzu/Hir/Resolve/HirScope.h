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

enum class [[nodiscard]] HirScopeKind : uint8_t { Func, Block };

/// A lexical scope's bindings.
class [[nodiscard]] HirScope {
public:
  // `const Ident *` is a query row alias (`from t e` binds `e`); its row type
  // lives in the type side table keyed by that `Ident`, so it resolves through
  // the same `typeOf(decl)` path as a `let`/param.
  using Binding = std::variant<const LetStmt *, const Param *, const FuncStmt *,
                               const Ident *>;

  using LookupResult = std::optional<Binding>;

  explicit HirScope(HirContext &ctx, HirScopeKind kind)
      : ctx(ctx), kind(kind) {}

  HirScope(const HirScope &) = delete;
  HirScope &operator=(const HirScope &) = delete;
  HirScope(HirScope &&) = delete;
  HirScope &operator=(HirScope &&) = delete;

  HirScopeKind getKind() const { return kind; }

  // Value namespace: `let`/`param`/`fn` declarations.
  void bind(const Ident *ident, Binding decl);

  LookupResult lookup(const Ident *ident) const {
    const auto it = bindings.find(ident->getName());
    if (it == bindings.end()) {
      return std::nullopt;
    }
    return it->second;
  }

  // types::Type namespace (a function's `[T]` params today), separate from the
  // value namespace so a value `T` and a type `T` don't collide.
  void bindType(std::u32string_view name, const types::Type *type) {
    types[name] = type;
  }

  const types::Type *lookupType(std::u32string_view name) const {
    const auto it = types.find(name);
    return it == types.end() ? nullptr : it->second;
  }

  // Table namespace: relations registered by `table` declarations. Kept apart
  // from the value namespace so a table is reachable only from a query's
  // `from`, never as an ordinary host value.
  void bindTable(std::u32string_view name,
                 const types::RelationType *relation) {
    tables[name] = relation;
  }

  const types::RelationType *lookupTable(std::u32string_view name) const {
    const auto it = tables.find(name);
    return it == tables.end() ? nullptr : it->second;
  }

private:
  llvm::DenseMap<std::u32string_view, Binding> bindings;
  llvm::DenseMap<std::u32string_view, const types::Type *> types;
  llvm::DenseMap<std::u32string_view, const types::RelationType *> tables;
  HirContext &ctx;
  HirScopeKind kind;
};

/// RAII handle for a pushed scope: pops it off the symbol table's stack when
/// the guard leaves C++ scope.
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
