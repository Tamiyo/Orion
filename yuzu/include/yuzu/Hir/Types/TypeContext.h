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

  [[nodiscard]] TraitRegistry &getTraitRegistry() { return traits; }
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
    const Type *boolType = typeFactory.getBoolType();

    // Declare each operator trait and the result it yields over Self: null
    // means Self (arithmetic), `bool` for comparison/logical.
    for (std::u32string_view t :
         {U"Add", U"Sub", U"Mul", U"Div", U"Pow", U"Pos", U"Neg"}) {
      traits.registerTrait(t, nullptr);
    }
    for (std::u32string_view t :
         {U"Eq", U"Neq", U"Lt", U"Lte", U"Gt", U"Gte", U"And", U"Or", U"Not"}) {
      traits.registerTrait(t, boolType);
    }

    // Numerics: arithmetic, unary +/-, equality, and ordering.
    const Type *numerics[] = {
        typeFactory.getInt8Type(),    typeFactory.getInt16Type(),
        typeFactory.getInt32Type(),   typeFactory.getInt64Type(),
        typeFactory.getUInt8Type(),   typeFactory.getUInt16Type(),
        typeFactory.getUInt32Type(),  typeFactory.getUInt64Type(),
        typeFactory.getFloat32Type(), typeFactory.getFloat64Type()};
    for (const Type *n : numerics) {
      for (std::u32string_view t :
           {U"Add", U"Sub", U"Mul", U"Div", U"Pow", U"Pos", U"Neg", U"Eq",
            U"Neq", U"Lt", U"Lte", U"Gt", U"Gte"}) {
        traits.registerImpl(t, n);
      }
    }

    // str: concatenation (`+`), equality, and ordering.
    for (std::u32string_view t :
         {U"Add", U"Eq", U"Neq", U"Lt", U"Lte", U"Gt", U"Gte"}) {
      traits.registerImpl(t, typeFactory.getStrType());
    }

    // bool: logical, equality, and `not`.
    for (std::u32string_view t : {U"And", U"Or", U"Not", U"Eq", U"Neq"}) {
      traits.registerImpl(t, boolType);
    }
  }

  TypeFactory typeFactory;
  TypeUnifier unifier;
  util::SideTable<HirId, const Type *> typeTable;
  TraitRegistry traits;
};
} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPECONTEXT_H