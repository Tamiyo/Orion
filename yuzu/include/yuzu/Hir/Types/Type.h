#ifndef YUZU_HIR_TYPES_TYPES_H
#define YUZU_HIR_TYPES_TYPES_H

#include "yuzu/Util/ErrorHandling.h"

#include <llvm/ADT/ArrayRef.h>

#include <cfloat>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>

namespace yuzu::hir {

//===----------------------------------------------------------------------===//
// Type hierarchy
//===----------------------------------------------------------------------===//
//
// `Type` is the base of an immutable, kind-discriminated hierarchy
// representing yuzu's types. Instances are owned by a `TypeContext`
// arena and handed out as `const Type *`; pointer equality is the same
// as structural equality once the context has interned the type. A
// typeck pass writes types into a `SideTable<HirId, const Type *>`; HIR
// itself stays type-free.

/// Discriminator carried by every `Type`. One enumerator per concrete
/// subclass; `Error` represents a typed "we already reported a problem
/// here, treat the value as unknown and keep going" so downstream
/// passes don't have to special-case nullable types.
enum class [[nodiscard]] TypeKind : uint8_t {
  Int8,
  Int16,
  Int32,
  Int64,
  UInt8,
  UInt16,
  UInt32,
  UInt64,
  Float32,
  Float64,
  Bool,
  Str,
  Relation,
  Struct,
  Func,
  TypeParam,
  Infer,
  //   Unit,
  //   List,
  //   Tuple,
  //   Class,
  Error,
};

/// Kind of an `InferTy` variable.
enum class [[nodiscard]] InferKind : uint8_t {
  General,
  Int,
  Float,
};

/// Human-facing name for a `TypeKind`. Rendered into diagnostic
/// messages — e.g. "operand has type `int`".
inline std::string asString(TypeKind kind) {
  switch (kind) {
  case TypeKind::Int8:
    return "int8";
  case TypeKind::Int16:
    return "int16";
  case TypeKind::Int32:
    return "int32";
  case TypeKind::Int64:
    return "int64";
  case TypeKind::UInt8:
    return "uint8";
  case TypeKind::UInt16:
    return "uint16";
  case TypeKind::UInt32:
    return "uint32";
  case TypeKind::UInt64:
    return "uint64";
  case TypeKind::Float32:
    return "float32";
  case TypeKind::Float64:
    return "float64";
  case TypeKind::Bool:
    return "bool";
  case TypeKind::Str:
    return "str";
  case TypeKind::Relation:
    return "relation";
  case TypeKind::Struct:
    return "struct";
  case TypeKind::Func:
    return "func";
  case TypeKind::TypeParam:
    return "<type-param>";
  case TypeKind::Infer:
    return "<infer>";
  case TypeKind::Error:
    return "<error>";
  }

  util::yuzu_unreachable();
}

/// Root of the type hierarchy. Stores the `TypeKind` discriminator;
/// concrete subclasses add per-kind payload. The constructor is
/// `protected` so callers go through `TypeContext` (which interns and
/// returns canonical pointers) rather than building `Type`s directly.
class [[nodiscard]] Type {
public:
  [[nodiscard]] TypeKind getKind() const { return kind; }

  [[nodiscard]] bool isNumeric() const {
    switch (kind) {
    case TypeKind::Int8:
    case TypeKind::Int16:
    case TypeKind::Int32:
    case TypeKind::Int64:
    case TypeKind::UInt8:
    case TypeKind::UInt16:
    case TypeKind::UInt32:
    case TypeKind::UInt64:
    case TypeKind::Float32:
    case TypeKind::Float64:
      return true;
    default:
      return false;
    }
  }

  [[nodiscard]] bool isInt() const {
    switch (kind) {
    case TypeKind::Int8:
    case TypeKind::Int16:
    case TypeKind::Int32:
    case TypeKind::Int64:
    case TypeKind::UInt8:
    case TypeKind::UInt16:
    case TypeKind::UInt32:
    case TypeKind::UInt64:
      return true;
    default:
      return false;
    }
  }

  [[nodiscard]] bool isUnsigned() const {
    switch (kind) {
    case TypeKind::UInt8:
    case TypeKind::UInt16:
    case TypeKind::UInt32:
    case TypeKind::UInt64:
      return true;
    default:
      return false;
    }
  }

  [[nodiscard]] bool isSigned() const {
    switch (kind) {
    case TypeKind::Int8:
    case TypeKind::Int16:
    case TypeKind::Int32:
    case TypeKind::Int64:
    case TypeKind::Float32:
    case TypeKind::Float64:
      return true;
    default:
      return false;
    }
  }

  [[nodiscard]] bool isFloat() const {
    switch (kind) {
    case TypeKind::Float32:
    case TypeKind::Float64:
      return true;
    default:
      return false;
    }
  }

  /// An unresolved inference hole (an `InferTy`) — a literal or generic
  /// result the solve pass hasn't pinned to a concrete type yet.
  [[nodiscard]] bool isHole() const { return kind == TypeKind::Infer; }

  /// Whether this integer type can represent `value`. Non-integer types
  /// return false. (`int64`/`uint64` hold any in-range `int64_t`.)
  [[nodiscard]] bool canRepresent(int64_t value) const {
    switch (kind) {
    case TypeKind::Int8:
      return value >= INT8_MIN && value <= INT8_MAX;
    case TypeKind::Int16:
      return value >= INT16_MIN && value <= INT16_MAX;
    case TypeKind::Int32:
      return value >= INT32_MIN && value <= INT32_MAX;
    case TypeKind::Int64:
      return true;
    case TypeKind::UInt8:
      return value >= 0 && value <= UINT8_MAX;
    case TypeKind::UInt16:
      return value >= 0 && value <= UINT16_MAX;
    case TypeKind::UInt32:
      return value >= 0 && value <= UINT32_MAX;
    case TypeKind::UInt64:
      return value >= 0;
    default:
      return false;
    }
  }

  /// Whether this float type can represent `value`'s magnitude. `float64`
  /// holds any double; `float32` rejects magnitudes past its finite range.
  /// Non-float types return false.
  [[nodiscard]] bool canRepresent(double value) const {
    switch (kind) {
    case TypeKind::Float32:
      return std::abs(value) <= static_cast<double>(FLT_MAX);
    case TypeKind::Float64:
      return true;
    default:
      return false;
    }
  }

protected:
  explicit Type(TypeKind k) : kind(k) {}

private:
  TypeKind kind;
};

/// LLVM-style RTTI for a `Type` subclass keyed on `TypeKind::<KindValue>`.
/// Emits the `isA(kind)` predicate and the `cast(const Type *)` downcast.
/// The class still spells its own constructor — this macro covers only
/// the boilerplate that's identical across every concrete leaf.
#define YUZU_TYPE_RTTI(KindValue)                                              \
  [[nodiscard]] static bool isA(TypeKind k) {                                  \
    return k == TypeKind::KindValue;                                           \
  }                                                                            \
                                                                               \
  [[nodiscard]] static const KindValue##Ty *cast(const Type *t) {              \
    return isA(t->getKind()) ? static_cast<const KindValue##Ty *>(t)           \
                             : nullptr;                                        \
  }

class Int8Ty final : public Type {
public:
  explicit Int8Ty() : Type(TypeKind::Int8) {}
  YUZU_TYPE_RTTI(Int8)
};

class Int16Ty final : public Type {
public:
  explicit Int16Ty() : Type(TypeKind::Int16) {}
  YUZU_TYPE_RTTI(Int16)
};

class Int32Ty final : public Type {
public:
  explicit Int32Ty() : Type(TypeKind::Int32) {}
  YUZU_TYPE_RTTI(Int32)
};

class Int64Ty final : public Type {
public:
  explicit Int64Ty() : Type(TypeKind::Int64) {}
  YUZU_TYPE_RTTI(Int64)
};

class UInt8Ty final : public Type {
public:
  explicit UInt8Ty() : Type(TypeKind::UInt8) {}
  YUZU_TYPE_RTTI(UInt8)
};

class UInt16Ty final : public Type {
public:
  explicit UInt16Ty() : Type(TypeKind::UInt16) {}
  YUZU_TYPE_RTTI(UInt16)
};

class UInt32Ty final : public Type {
public:
  explicit UInt32Ty() : Type(TypeKind::UInt32) {}
  YUZU_TYPE_RTTI(UInt32)
};

class UInt64Ty final : public Type {
public:
  explicit UInt64Ty() : Type(TypeKind::UInt64) {}
  YUZU_TYPE_RTTI(UInt64)
};

class Float32Ty final : public Type {
public:
  explicit Float32Ty() : Type(TypeKind::Float32) {}
  YUZU_TYPE_RTTI(Float32)
};

class Float64Ty final : public Type {
public:
  explicit Float64Ty() : Type(TypeKind::Float64) {}
  YUZU_TYPE_RTTI(Float64)
};

class BoolTy final : public Type {
public:
  explicit BoolTy() : Type(TypeKind::Bool) {}
  YUZU_TYPE_RTTI(Bool)
};

class StrTy final : public Type {
public:
  explicit StrTy() : Type(TypeKind::Str) {}
  YUZU_TYPE_RTTI(Str)
};

class ErrorTy final : public Type {
public:
  explicit ErrorTy() : Type(TypeKind::Error) {}
  YUZU_TYPE_RTTI(Error)
};

/// `Relation[Element]` — a relation over rows of type `Element`. Interned
/// structurally by its element.
class RelationTy final : public Type {
public:
  explicit RelationTy(const Type *element)
      : Type(TypeKind::Relation), element(element) {}

  [[nodiscard]] const Type *getElement() const { return element; }

  YUZU_TYPE_RTTI(Relation)

private:
  const Type *element;
};

/// A `StructTy` field: interned name + type.
struct Field {
  std::u32string_view name;
  const Type *type;
};

/// A nominal record type — a declared `struct`.
class StructTy final : public Type {
public:
  StructTy(std::u32string_view name, llvm::ArrayRef<Field> fields)
      : Type(TypeKind::Struct), name(name), fields(fields) {}

  [[nodiscard]] std::u32string_view getName() const { return name; }
  [[nodiscard]] llvm::ArrayRef<Field> getFields() const { return fields; }

  /// Type of the field named `fieldName`, or null if there is none.
  [[nodiscard]] const Type *findField(std::u32string_view fieldName) const {
    for (const Field &f : fields) {
      if (f.name == fieldName) {
        return f.type;
      }
    }
    return nullptr;
  }

  YUZU_TYPE_RTTI(Struct)

private:
  std::u32string_view name;
  llvm::ArrayRef<Field> fields;
};

/// A function type `(P0, ...) -> R`. The `params` array is arena-owned. A
/// generic signature is a template: its `[T]` params are `TypeParamTy`
/// markers a call site instantiates.
class FuncTy final : public Type {
public:
  FuncTy(llvm::ArrayRef<const Type *> params, const Type *ret)
      : Type(TypeKind::Func), params(params), ret(ret) {}

  [[nodiscard]] llvm::ArrayRef<const Type *> getParams() const {
    return params;
  }
  [[nodiscard]] const Type *getRet() const { return ret; }

  YUZU_TYPE_RTTI(Func)

private:
  llvm::ArrayRef<const Type *> params;
  const Type *ret;
};

/// A declared generic type parameter — the `T` in `fn id[T](...)`. A rigid
/// marker, never unified: a call site substitutes a fresh hole for it.
/// `index` is its position in `[...]`; `name` is for diagnostics.
class TypeParamTy final : public Type {
public:
  TypeParamTy(uint32_t index, std::u32string_view name)
      : Type(TypeKind::TypeParam), index(index), name(name) {}

  [[nodiscard]] uint32_t getIndex() const { return index; }
  [[nodiscard]] std::u32string_view getName() const { return name; }

  YUZU_TYPE_RTTI(TypeParam)

private:
  uint32_t index;
  std::u32string_view name;
};

/// Identifies an inference variable within a `UnificationTable`.
enum class InferId : uint32_t {};

/// An unresolved inference variable, owned by a `UnificationTable`. Its
/// `id` indexes the table's union-find; `flavor` restricts what it can
/// unify with and how it defaults. Replaced by a concrete type during the
/// solve, so it never escapes into a finished tree.
class InferTy final : public Type {
public:
  InferTy(InferId id, InferKind inferKind)
      : Type(TypeKind::Infer), id(id), inferKind(inferKind) {}

  [[nodiscard]] InferId getId() const { return id; }
  [[nodiscard]] InferKind getInferKind() const { return inferKind; }

  YUZU_TYPE_RTTI(Infer)

private:
  InferId id;
  InferKind inferKind;
};
} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPES_H
