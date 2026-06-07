#ifndef YUZU_HIR_OPS_TRAIT_H
#define YUZU_HIR_OPS_TRAIT_H

#include "yuzu/Hir/Types/Type.h"

#include <llvm/ADT/DenseMap.h>

#include <cstdint>
#include <optional>
#include <utility>

namespace yuzu::hir {

/// An operation an operator stands for. Internal only (no user syntax yet):
/// a builtin operator maps to a trait, and the `TraitTable` records which
/// types implement it. The eventual user-facing `interface`/`impl` system
/// will populate the same table instead of the startup loop.
enum class [[nodiscard]] Trait : uint8_t {
  Add,
  Sub,
  Mul,
  Div,
  Pow,
  And,
  Or,
  Eq,
  Neq,
  Lt,
  Lte,
  Gt,
  Gte,
  Pos,
  Neg,
  Not,
};

/// Records which types implement which traits, as `(trait, operand) → result`
/// rows. A null `result` means "same as the operand" (so an arithmetic op on
/// an open literal hole keeps the hole); a non-null result is fixed (e.g.
/// comparisons yield `bool`). Replaces per-operator type switches with data.
class TraitTable {
public:
  /// `result == nullptr` means the operation yields the operand's own type.
  void add(Trait trait, const Type *operand, const Type *result) {
    rows[{trait, operand}] = result;
  }

  /// `nullopt` if `operand` does not implement `trait`. Otherwise the result
  /// type, where a null payload means "same as the operand".
  [[nodiscard]] std::optional<const Type *> lookup(Trait trait,
                                                   const Type *operand) const {
    const auto it = rows.find({trait, operand});
    if (it == rows.end()) {
      return std::nullopt;
    }
    return it->second;
  }

private:
  llvm::DenseMap<std::pair<Trait, const Type *>, const Type *> rows;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_OPS_TRAIT_H