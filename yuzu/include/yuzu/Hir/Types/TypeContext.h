#ifndef YUZU_HIR_TYPES_TYPECONTEXT_H
#define YUZU_HIR_TYPES_TYPECONTEXT_H

#include "yuzu/Hir/Hir.h"
#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Hir/Types/TypeFactory.h"
#include "yuzu/Hir/Types/TypeUnifier.h"
#include "yuzu/Util/SideTable.h"
#include "yuzu/Util/StringInterner.h"

#include <llvm/ADT/ArrayRef.h>

#include <string_view>

namespace yuzu::hir {
/// One handle for an HIR tree's type system: the type factory (interner),
/// the node → type record, and the hole unifier. Composes the three so
/// callers reach them through one object instead of juggling each.
class TypeContext {
public:
  explicit TypeContext(util::StringInterner &strings)
      : i(strings), unifier(i) {}

  [[nodiscard]] const Int8Ty *getInt8() const { return i.getInt8(); }
  [[nodiscard]] const Int16Ty *getInt16() const { return i.getInt16(); }
  [[nodiscard]] const Int32Ty *getInt32() const { return i.getInt32(); }
  [[nodiscard]] const Int64Ty *getInt64() const { return i.getInt64(); }
  [[nodiscard]] const UInt8Ty *getUInt8() const { return i.getUInt8(); }
  [[nodiscard]] const UInt16Ty *getUInt16() const { return i.getUInt16(); }
  [[nodiscard]] const UInt32Ty *getUInt32() const { return i.getUInt32(); }
  [[nodiscard]] const UInt64Ty *getUInt64() const { return i.getUInt64(); }
  [[nodiscard]] const Float32Ty *getFloat32() const { return i.getFloat32(); }
  [[nodiscard]] const Float64Ty *getFloat64() const { return i.getFloat64(); }
  [[nodiscard]] const BoolTy *getBool() const { return i.getBool(); }
  [[nodiscard]] const StrTy *getStr() const { return i.getStr(); }
  [[nodiscard]] const ErrorTy *getError() const { return i.getError(); }

  [[nodiscard]] const RelationTy *getRelation(const Type *element) {
    return i.getRelation(element);
  }

  const StructTy *getStruct(std::u32string_view name,
                            llvm::ArrayRef<Field> fields) {
    return i.getStruct(name, fields);
  }

  const FuncTy *getFunc(llvm::ArrayRef<const Type *> params, const Type *ret) {
    return i.getFunc(params, ret);
  }

  [[nodiscard]] const Type *resolveNamed(std::u32string_view name) const {
    return i.resolveNamed(name);
  }

  // Record — a node's type, which may be a hole until concretized.
  void bind(const HirNode *node, const Type *type) {
    table.bind(node->getId(), type);
  }

  [[nodiscard]] const Type *typeOf(const HirNode *node) const {
    const auto *t = table.get(node->getId());
    return t ? *t : nullptr;
  }

  // Solver.
  const InferTy *hole(InferKind kind) { return unifier.makeTypeHole(kind); }

  bool unify(const Type *a, const Type *b) { return unifier.unify(a, b); }

  const Type *resolve(const Type *t) { return unifier.resolve(t); }

  /// Resolve `node`'s recorded type in place — its hole follows its fill or
  /// defaults — and return the concrete result. Null if `node` is untyped.
  const Type *concretize(const HirNode *node) {
    const Type *t = typeOf(node);
    if (!t) {
      return nullptr;
    }
    const Type *resolved = unifier.resolve(t);
    bind(node, resolved);
    return resolved;
  }

private:
  TypeFactory i;
  TypeUnifier unifier;
  util::SideTable<HirId, const Type *> table;
};
} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPECONTEXT_H