#ifndef YUZU_HIR_TYPES_TYPEINTERNER_H
#define YUZU_HIR_TYPES_TYPEINTERNER_H

#include "yuzu/Hir/Types/Type.h"

#include <llvm/Support/Allocator.h>

namespace yuzu::hir {
class TypeInterner {
public:
  TypeInterner() {
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
  }

  TypeInterner(const TypeInterner &) = delete;
  TypeInterner &operator=(const TypeInterner &) = delete;

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

  // Compound types — hash-cons via the interner.
  // const ListTy *getList(const Type *elem);
  // const FuncTy *getFunc(llvm::ArrayRef<const Type *> params,
  //                       const Type *result);

private:
  llvm::BumpPtrAllocator arena;
  // llvm::DenseMap<TypeKey, const Type *> interner;  // when compounds land

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
