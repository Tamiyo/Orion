#ifndef YUZU_HIR_OPS_TRAIT_H
#define YUZU_HIR_OPS_TRAIT_H

#include "yuzu/Types/Type.h"
#include "yuzu/Util/U32StringExtensions.h" // IWYU pragma: keep (DenseMap key)

#include <llvm/ADT/DenseMap.h>

#include <optional>
#include <string_view>
#include <utility>

namespace yuzu::hir {

/// Which traits exist and which types implement them. A trait is named (e.g.
/// `Add`), not a closed enum, so the builtin operator traits and a future
/// user-facing `trait`/`impl` system share one mechanism: builtins are
/// registered at startup, user declarations register into the same maps.
///
/// A trait's result is a `const types::Type *`, where null means Self (the
/// implementing type). That generalizes — a result may be Self (`Add`:
/// `T + T -> T`), a fixed type (`Eq` -> `bool`), or eventually a parameter of
/// the trait itself (`Blah[K]` -> `K`); a closed set of result shapes would
/// not.
class TraitRegistry {
public:
  /// Register that `trait` exists, yielding `result` (null = Self) by default.
  void registerTrait(std::u32string_view trait, const types::Type *result) {
    traits.insert({trait, result});
  }

  [[nodiscard]] bool isRegistered(std::u32string_view trait) const {
    return traits.contains(trait);
  }

  /// Register that `operand` implements `trait`, yielding the trait's own
  /// result (so the impl matches what the trait declares).
  void registerImpl(std::u32string_view trait, const types::Type *operand) {
    registerImpl(trait, operand, traits.lookup(trait));
  }

  /// Register an impl with an explicit `result` (null = same as the operand),
  /// for impls that override the trait's default result type.
  void registerImpl(std::u32string_view trait, const types::Type *operand,
                    const types::Type *result) {
    impls.insert({{trait, operand}, result});
  }

  /// The result of applying `trait` to `operand`, or nullopt if `operand` does
  /// not implement it. A null payload means "same as the operand".
  [[nodiscard]] std::optional<const types::Type *>
  lookupImpl(std::u32string_view trait, const types::Type *operand) const {
    const auto it = impls.find({trait, operand});
    if (it == impls.end()) {
      return std::nullopt;
    }
    return it->second;
  }

private:
  // Names are interned (or string literals for builtins), so the views stay
  // valid for the registry's lifetime.
  llvm::DenseMap<std::u32string_view, const types::Type *> traits;
  llvm::DenseMap<std::pair<std::u32string_view, const types::Type *>,
                 const types::Type *>
      impls;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_OPS_TRAIT_H