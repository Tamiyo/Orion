#ifndef YUZU_HIR_TYPES_TYPECONTEXT_H
#define YUZU_HIR_TYPES_TYPECONTEXT_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/Ops/Trait.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeFactory.h"
#include "yuzu/Hir/Types/TypeUnifier.h"
#include "yuzu/Util/SideTable.h"
#include "yuzu/Util/StringInterner.h"

#include <llvm/ADT/ArrayRef.h>

#include <string_view>

namespace yuzu::hir {
class TypeContext {
public:
  explicit TypeContext(util::StringInterner &strings)
      : typeFactory(strings), unifier(typeFactory) {
    registerBuiltinTraits();
  }

  [[nodiscard]] TraitTable &getTraitTable() { return traits; }
  [[nodiscard]] TypeFactory &getTypeFactory() { return typeFactory; }

  // Record — a node's type, which may be a hole until concretized.
  void bind(const HirNode *node, const Type *type) {
    typeTable.bind(node->getId(), type);
  }

  [[nodiscard]] const Type *typeOf(const HirNode *node) const {
    const auto *type = typeTable.get(node->getId());
    return type ? *type : nullptr;
  }

  // Solver.
  const InferType *makeTypeHole(InferKind kind) {
    return unifier.makeTypeHole(kind);
  }

  bool unifyTypes(const Type *a, const Type *b) { return unifier.unify(a, b); }

  const Type *resolveType(const Type *t) { return unifier.resolve(t); }

  /// Resolve `node`'s recorded type in place — its hole follows its fill or
  /// defaults — and return the concrete result. Null if `node` is untyped.
  const Type *concretize(const HirNode *node) {
    const Type *type = typeOf(node);
    if (!type) {
      return nullptr;
    }
    const Type *resolved = unifier.resolve(type);
    bind(node, resolved);
    return resolved;
  }

private:
  /// Register the builtin trait impls: which types support which operators.
  /// A null result means "same type as the operand" (arithmetic, unary `-`);
  /// a fixed result is given explicitly (comparisons/logical → bool).
  void registerBuiltinTraits() {
    const Type *numerics[] = {
        typeFactory.getInt8Type(),    typeFactory.getInt16Type(),
        typeFactory.getInt32Type(),   typeFactory.getInt64Type(),
        typeFactory.getUInt8Type(),   typeFactory.getUInt16Type(),
        typeFactory.getUInt32Type(),  typeFactory.getUInt64Type(),
        typeFactory.getFloat32Type(), typeFactory.getFloat64Type()};

    for (const Type *n : numerics) {
      // Arithmetic and unary +/- preserve the operand type (null result).
      traits.add(Trait::Add, n, nullptr);
      traits.add(Trait::Sub, n, nullptr);
      traits.add(Trait::Mul, n, nullptr);
      traits.add(Trait::Div, n, nullptr);
      traits.add(Trait::Pow, n, nullptr);
      traits.add(Trait::Pos, n, nullptr);
      traits.add(Trait::Neg, n, nullptr);
      // Equality and ordering yield bool.
      traits.add(Trait::Eq, n, typeFactory.getBoolType());
      traits.add(Trait::Neq, n, typeFactory.getBoolType());
      traits.add(Trait::Lt, n, typeFactory.getBoolType());
      traits.add(Trait::Lte, n, typeFactory.getBoolType());
      traits.add(Trait::Gt, n, typeFactory.getBoolType());
      traits.add(Trait::Gte, n, typeFactory.getBoolType());
    }

    // str: concatenation (`+`), equality, and ordering.
    traits.add(Trait::Add, typeFactory.getStrType(), nullptr);
    traits.add(Trait::Eq, typeFactory.getStrType(), typeFactory.getBoolType());
    traits.add(Trait::Neq, typeFactory.getStrType(), typeFactory.getBoolType());
    traits.add(Trait::Lt, typeFactory.getStrType(), typeFactory.getBoolType());
    traits.add(Trait::Lte, typeFactory.getStrType(), typeFactory.getBoolType());
    traits.add(Trait::Gt, typeFactory.getStrType(), typeFactory.getBoolType());
    traits.add(Trait::Gte, typeFactory.getStrType(), typeFactory.getBoolType());

    // bool: logical, equality, and `not`.
    traits.add(Trait::And, typeFactory.getBoolType(),
               typeFactory.getBoolType());
    traits.add(Trait::Or, typeFactory.getBoolType(), typeFactory.getBoolType());
    traits.add(Trait::Eq, typeFactory.getBoolType(), typeFactory.getBoolType());
    traits.add(Trait::Neq, typeFactory.getBoolType(),
               typeFactory.getBoolType());
    traits.add(Trait::Not, typeFactory.getBoolType(),
               typeFactory.getBoolType());
  }

  TypeFactory typeFactory;
  TypeUnifier unifier;
  util::SideTable<HirId, const Type *> typeTable;
  TraitTable traits;
};
} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPECONTEXT_H