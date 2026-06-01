#ifndef YUZU_HIR_TYPES_TYPEFACTORY_H
#define YUZU_HIR_TYPES_TYPEFACTORY_H

#include "yuzu/Hir/Types/Type.h"
#include "yuzu/Util/StringInterner.h"

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/Support/Allocator.h>

#include <cstddef>
#include <string_view>

namespace yuzu::hir {
class TypeFactory {
public:
  explicit TypeFactory(util::StringInterner &strings)
      : stringInterner(strings) {
    // Pre-allocate primitives in the arena so getXxx() returns a stable
    // pointer with no interner hit. Placement-new each one in turn.
    int8Ty = new (arena.Allocate<Int8Ty>()) Int8Ty();
    int16Ty = new (arena.Allocate<Int16Ty>()) Int16Ty();
    int32Ty = new (arena.Allocate<Int32Ty>()) Int32Ty();
    int64Ty = new (arena.Allocate<Int64Ty>()) Int64Ty();

    uint8Ty = new (arena.Allocate<UInt8Ty>()) UInt8Ty();
    uint16Ty = new (arena.Allocate<UInt16Ty>()) UInt16Ty();
    uint32Ty = new (arena.Allocate<UInt32Ty>()) UInt32Ty();
    uint64Ty = new (arena.Allocate<UInt64Ty>()) UInt64Ty();

    float32Ty = new (arena.Allocate<Float32Ty>()) Float32Ty();
    float64Ty = new (arena.Allocate<Float64Ty>()) Float64Ty();

    boolTy = new (arena.Allocate<BoolTy>()) BoolTy();
    strTy = new (arena.Allocate<StrTy>()) StrTy();
    errorTy = new (arena.Allocate<ErrorTy>()) ErrorTy();

    // Register the built-in type names so `resolveNamed` can map a source
    // spelling (`int64`, `bool`, ...) back to its type. Spellings match
    // `asString`. Constructor types (`Relation`, ...) are intentionally
    // absent: they take arguments and are built by the lowerer.
    typesByName = {
        {U"int8", int8Ty},       {U"int16", int16Ty},   {U"int32", int32Ty},
        {U"int64", int64Ty},     {U"uint8", uint8Ty},   {U"uint16", uint16Ty},
        {U"uint32", uint32Ty},   {U"uint64", uint64Ty}, {U"float32", float32Ty},
        {U"float64", float64Ty}, {U"bool", boolTy},     {U"str", strTy},
    };
  }

  TypeFactory(const TypeFactory &) = delete;
  TypeFactory &operator=(const TypeFactory &) = delete;

  // Primitives — pointer-stable, no allocation per call.
  [[nodiscard]] const Int8Ty *getInt8() const { return int8Ty; }
  [[nodiscard]] const Int16Ty *getInt16() const { return int16Ty; }
  [[nodiscard]] const Int32Ty *getInt32() const { return int32Ty; }
  [[nodiscard]] const Int64Ty *getInt64() const { return int64Ty; }

  [[nodiscard]] const UInt8Ty *getUInt8() const { return uint8Ty; }
  [[nodiscard]] const UInt16Ty *getUInt16() const { return uint16Ty; }
  [[nodiscard]] const UInt32Ty *getUInt32() const { return uint32Ty; }
  [[nodiscard]] const UInt64Ty *getUInt64() const { return uint64Ty; }

  [[nodiscard]] const Float32Ty *getFloat32() const { return float32Ty; }
  [[nodiscard]] const Float64Ty *getFloat64() const { return float64Ty; }

  [[nodiscard]] const BoolTy *getBool() const { return boolTy; }
  [[nodiscard]] const StrTy *getStr() const { return strTy; }
  [[nodiscard]] const ErrorTy *getError() const { return errorTy; }

  // Compound types — hash-cons via the interner so structurally equal
  // types share one canonical pointer.

  /// `Relation[element]`, deduplicated by element pointer. Two calls with
  /// the same (already-interned) element return the same `RelationTy`.
  [[nodiscard]] const RelationTy *getRelation(const Type *element) {
    auto [it, inserted] = relations.try_emplace(element, nullptr);
    if (inserted) {
      it->second = new (arena.Allocate<RelationTy>()) RelationTy(element);
    }
    return it->second;
  }

  /// `(params...) -> ret`. Not interned: a function signature isn't
  /// compared by pointer, and a generic function's params/ret hold unique
  /// inference holes that must not be shared, so each call allocates a
  /// fresh `FuncTy` with its `params` copied into the arena.
  const FuncTy *getFunc(llvm::ArrayRef<const Type *> params, const Type *ret) {
    const Type **savedParams = arena.Allocate<const Type *>(params.size());
    for (size_t i = 0; i < params.size(); ++i) {
      savedParams[i] = params[i];
    }
    return new (arena.Allocate<FuncTy>())
        FuncTy(llvm::ArrayRef<const Type *>(savedParams, params.size()), ret);
  }

  /// Fresh nominal struct, registered for `resolveNamed`. Name and fields
  /// are interned/copied, so the caller's storage needn't outlive the call.
  const StructTy *getStruct(std::u32string_view name,
                            llvm::ArrayRef<Field> fields) {
    // Get raw memory from the arena.
    Field *savedFields = arena.Allocate<Field>(fields.size());

    // Allocate the memory.
    for (size_t i = 0; i < fields.size(); ++i) {
      savedFields[i] =
          Field{stringInterner.intern(fields[i].name), fields[i].type};
    }

    // Construt the type.
    const auto *ty = new (arena.Allocate<StructTy>())
        StructTy(stringInterner.intern(name),
                 llvm::ArrayRef<Field>(savedFields, fields.size()));

    typesByName[stringInterner.intern(name)] = ty;
    return ty;
  }

  /// Resolve a type name to its type, or null. Constructor types like
  /// `Relation[...]` aren't here — the lowerer builds those from their args.
  [[nodiscard]] const Type *resolveNamed(std::u32string_view name) const {
    const auto it = typesByName.find(name);
    return it == typesByName.end() ? nullptr : it->second;
  }

private:
  llvm::BumpPtrAllocator arena;

  // Interned compounds, keyed by their structural payload.
  llvm::DenseMap<const Type *, const RelationTy *> relations;

  // Atomic type names (built-ins + declared structs) for `resolveNamed`,
  // keyed by interned name views.
  llvm::DenseMap<std::u32string_view, const Type *> typesByName;

  // Shared string storage for struct/field names. Owned by the HIR
  // context, not the type arena, so names dedup across the whole HIR.
  util::StringInterner &stringInterner;

  const Int8Ty *int8Ty;
  const Int16Ty *int16Ty;
  const Int32Ty *int32Ty;
  const Int64Ty *int64Ty;
  const UInt8Ty *uint8Ty;
  const UInt16Ty *uint16Ty;
  const UInt32Ty *uint32Ty;
  const UInt64Ty *uint64Ty;
  const Float32Ty *float32Ty;
  const Float64Ty *float64Ty;
  const BoolTy *boolTy;
  const StrTy *strTy;
  const ErrorTy *errorTy;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPE_CONTEXT_H
