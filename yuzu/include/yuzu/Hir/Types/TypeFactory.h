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
    int8Type= new (arena.Allocate<Int8Type>()) Int8Type();
    int16Type= new (arena.Allocate<Int16Type>()) Int16Type();
    int32Type= new (arena.Allocate<Int32Type>()) Int32Type();
    int64Type= new (arena.Allocate<Int64Type>()) Int64Type();

    uint8Type= new (arena.Allocate<UInt8Type>()) UInt8Type();
    uint16Type= new (arena.Allocate<UInt16Type>()) UInt16Type();
    uint32Type= new (arena.Allocate<UInt32Type>()) UInt32Type();
    uint64Type= new (arena.Allocate<UInt64Type>()) UInt64Type();

    float32Type= new (arena.Allocate<Float32Type>()) Float32Type();
    float64Type= new (arena.Allocate<Float64Type>()) Float64Type();

    boolType= new (arena.Allocate<BoolType>()) BoolType();
    strType= new (arena.Allocate<StrType>()) StrType();
    unitType= new (arena.Allocate<UnitType>()) UnitType();
    errorType= new (arena.Allocate<ErrorType>()) ErrorType();
  }

  TypeFactory(const TypeFactory &) = delete;
  TypeFactory &operator=(const TypeFactory &) = delete;

  // Primitives — pointer-stable, no allocation per call.
  [[nodiscard]] const Int8Type *getInt8Type() const { return int8Type; }
  [[nodiscard]] const Int16Type *getInt16Type() const { return int16Type; }
  [[nodiscard]] const Int32Type *getInt32Type() const { return int32Type; }
  [[nodiscard]] const Int64Type *getInt64Type() const { return int64Type; }

  [[nodiscard]] const UInt8Type *getUInt8Type() const { return uint8Type; }
  [[nodiscard]] const UInt16Type *getUInt16Type() const { return uint16Type; }
  [[nodiscard]] const UInt32Type *getUInt32Type() const { return uint32Type; }
  [[nodiscard]] const UInt64Type *getUInt64Type() const { return uint64Type; }

  [[nodiscard]] const Float32Type *getFloat32Type() const { return float32Type; }
  [[nodiscard]] const Float64Type *getFloat64Type() const { return float64Type; }

  [[nodiscard]] const BoolType *getBoolType() const { return boolType; }
  [[nodiscard]] const StrType *getStrType() const { return strType; }
  [[nodiscard]] const UnitType *getUnitType() const { return unitType; }
  [[nodiscard]] const ErrorType *getErrorType() const { return errorType; }

  /// The interned scalar for a builtin `TypeKind` (those in `ScalarBuiltins`).
  [[nodiscard]] const Type *getScalarTy(TypeKind kind) const {
    switch (kind) {
    case TypeKind::Int8:
      return int8Type;
    case TypeKind::Int16:
      return int16Type;
    case TypeKind::Int32:
      return int32Type;
    case TypeKind::Int64:
      return int64Type;
    case TypeKind::UInt8:
      return uint8Type;
    case TypeKind::UInt16:
      return uint16Type;
    case TypeKind::UInt32:
      return uint32Type;
    case TypeKind::UInt64:
      return uint64Type;
    case TypeKind::Float32:
      return float32Type;
    case TypeKind::Float64:
      return float64Type;
    case TypeKind::Bool:
      return boolType;
    case TypeKind::Str:
      return strType;
    case TypeKind::Unit:
      return unitType;
    default:
      util::yuzu_unreachable();
    }
  }

  // Compound types — hash-cons via the interner so structurally equal
  // types share one canonical pointer.

  /// `Relation[element]`, deduplicated by element pointer. Two calls with
  /// the same (already-interned) element return the same `RelationTy`.
  [[nodiscard]] const RelationType *getRelationType(const Type *element) {
    auto [it, inserted] = relations.try_emplace(element, nullptr);
    if (inserted) {
      it->second = new (arena.Allocate<RelationType>()) RelationType(element);
    }
    return it->second;
  }

  /// `(params...) -> ret`. Not interned: a function signature isn't compared
  /// by pointer, so each call allocates a fresh `FuncTy` with its `params`
  /// copied into the arena.
  const FuncType *getFuncTy(llvm::ArrayRef<const Type *> params, const Type *ret) {
    const Type **savedParams = arena.Allocate<const Type *>(params.size());
    for (size_t i = 0; i < params.size(); ++i) {
      savedParams[i] = params[i];
    }
    return new (arena.Allocate<FuncType>())
        FuncType(llvm::ArrayRef<const Type *>(savedParams, params.size()), ret);
  }

  /// A generic type parameter `T#index`. Not interned: each declared `[T]`
  /// is a distinct marker (its identity, not its name, is what matters). The
  /// name is interned so it outlives the caller's storage.
  const TypeParamType *getTypeParamType(uint32_t index, std::u32string_view name) {
    return new (arena.Allocate<TypeParamType>())
        TypeParamType(index, stringInterner.intern(name));
  }

  /// Fresh nominal struct. Name and fields are interned/copied, so the
  /// caller's storage needn't outlive the call. Name *resolution* is the
  /// scope's job — the caller binds the struct's name into the environment.
  const StructType *getStructType(std::u32string_view name,
                            llvm::ArrayRef<StructField> fields) {
    StructField *savedFields = arena.Allocate<StructField>(fields.size());
    for (size_t i = 0; i < fields.size(); ++i) {
      savedFields[i] =
          StructField{stringInterner.intern(fields[i].name), fields[i].type};
    }
    return new (arena.Allocate<StructType>())
        StructType(stringInterner.intern(name),
                 llvm::ArrayRef<StructField>(savedFields, fields.size()));
  }

private:
  llvm::BumpPtrAllocator arena;

  // Interned compounds, keyed by their structural payload.
  llvm::DenseMap<const Type *, const RelationType *> relations;

  // Shared string storage for struct/field names. Owned by the HIR
  // context, not the type arena, so names dedup across the whole HIR.
  util::StringInterner &stringInterner;

  const Int8Type *int8Type;
  const Int16Type *int16Type;
  const Int32Type *int32Type;
  const Int64Type *int64Type;
  const UInt8Type *uint8Type;
  const UInt16Type *uint16Type;
  const UInt32Type *uint32Type;
  const UInt64Type *uint64Type;
  const Float32Type *float32Type;
  const Float64Type *float64Type;
  const BoolType *boolType;
  const StrType *strType;
  const UnitType *unitType;
  const ErrorType *errorType;
};

} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPE_CONTEXT_H
