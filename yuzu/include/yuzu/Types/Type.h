#ifndef YUZU_TYPES_TYPE_H
#define YUZU_TYPES_TYPE_H

#include "yuzu/Util/ErrorHandling.h"

#include <llvm/ADT/ArrayRef.h>

#include <array>
#include <cfloat>
#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace yuzu::types {

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
  Unit,
  Relation,
  Struct,
  Func,
  TypeParam,
  Infer,
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

/// The scalar builtin types and their source spellings — the single source
/// of truth for these names. `asString` renders from it, and the environment
/// seeds its root type scope from it (so `int32`/`bool`/... resolve there).
/// Compound (`Relation`) and synthetic (`<infer>`) kinds are not here: the
/// former are built from arguments, the latter have no source spelling.
inline constexpr std::array<std::pair<TypeKind, std::string_view>, 13>
    scalarBuiltins = {{
        {TypeKind::Int8, "int8"},
        {TypeKind::Int16, "int16"},
        {TypeKind::Int32, "int32"},
        {TypeKind::Int64, "int64"},
        {TypeKind::UInt8, "uint8"},
        {TypeKind::UInt16, "uint16"},
        {TypeKind::UInt32, "uint32"},
        {TypeKind::UInt64, "uint64"},
        {TypeKind::Float32, "float32"},
        {TypeKind::Float64, "float64"},
        {TypeKind::Bool, "bool"},
        {TypeKind::Str, "str"},
        {TypeKind::Unit, "unit"},
    }};

/// Human-facing name for a `TypeKind`. Rendered into diagnostic
/// messages — e.g. "operand has type `int32`".
inline std::string asString(TypeKind kind) {
  for (const auto &[k, name] : scalarBuiltins) {
    if (k == kind) {
      return std::string(name);
    }
  }
  switch (kind) {
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
  default:
    break;
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
  [[nodiscard]] static const KindValue##Type *cast(const Type *t) {            \
    return isA(t->getKind()) ? static_cast<const KindValue##Type *>(t)         \
                             : nullptr;                                        \
  }

class Int8Type final : public Type {
public:
  explicit Int8Type() : Type(TypeKind::Int8) {}
  YUZU_TYPE_RTTI(Int8)
};

class Int16Type final : public Type {
public:
  explicit Int16Type() : Type(TypeKind::Int16) {}
  YUZU_TYPE_RTTI(Int16)
};

class Int32Type final : public Type {
public:
  explicit Int32Type() : Type(TypeKind::Int32) {}
  YUZU_TYPE_RTTI(Int32)
};

class Int64Type final : public Type {
public:
  explicit Int64Type() : Type(TypeKind::Int64) {}
  YUZU_TYPE_RTTI(Int64)
};

class UInt8Type final : public Type {
public:
  explicit UInt8Type() : Type(TypeKind::UInt8) {}
  YUZU_TYPE_RTTI(UInt8)
};

class UInt16Type final : public Type {
public:
  explicit UInt16Type() : Type(TypeKind::UInt16) {}
  YUZU_TYPE_RTTI(UInt16)
};

class UInt32Type final : public Type {
public:
  explicit UInt32Type() : Type(TypeKind::UInt32) {}
  YUZU_TYPE_RTTI(UInt32)
};

class UInt64Type final : public Type {
public:
  explicit UInt64Type() : Type(TypeKind::UInt64) {}
  YUZU_TYPE_RTTI(UInt64)
};

class Float32Type final : public Type {
public:
  explicit Float32Type() : Type(TypeKind::Float32) {}
  YUZU_TYPE_RTTI(Float32)
};

class Float64Type final : public Type {
public:
  explicit Float64Type() : Type(TypeKind::Float64) {}
  YUZU_TYPE_RTTI(Float64)
};

class BoolType final : public Type {
public:
  explicit BoolType() : Type(TypeKind::Bool) {}
  YUZU_TYPE_RTTI(Bool)
};

class StrType final : public Type {
public:
  explicit StrType() : Type(TypeKind::Str) {}
  YUZU_TYPE_RTTI(Str)
};

class UnitType final : public Type {
public:
  explicit UnitType() : Type(TypeKind::Unit) {}
  YUZU_TYPE_RTTI(Unit)
};

class ErrorType final : public Type {
public:
  explicit ErrorType() : Type(TypeKind::Error) {}
  YUZU_TYPE_RTTI(Error)
};

/// `Relation[Element]` — a relation over rows of type `Element`. Interned
/// structurally by its element.
class RelationType final : public Type {
public:
  explicit RelationType(const Type *element)
      : Type(TypeKind::Relation), element(element) {}

  [[nodiscard]] const Type *getElement() const { return element; }

  YUZU_TYPE_RTTI(Relation)

private:
  const Type *element;
};

/// A `StructTy` field: interned name + type.
struct StructField {
  std::u32string_view name;
  const Type *type;
};

/// A nominal record type — a declared `struct`.
class StructType final : public Type {
public:
  StructType(std::u32string_view name, llvm::ArrayRef<StructField> fields)
      : Type(TypeKind::Struct), name(name), fields(fields) {}

  [[nodiscard]] std::u32string_view getName() const { return name; }
  [[nodiscard]] llvm::ArrayRef<StructField> getFields() const { return fields; }

  /// Type of the field named `fieldName`, or null if there is none.
  [[nodiscard]] const Type *findField(std::u32string_view fieldName) const {
    for (const StructField &f : fields) {
      if (f.name == fieldName) {
        return f.type;
      }
    }
    return nullptr;
  }

  YUZU_TYPE_RTTI(Struct)

private:
  std::u32string_view name;
  llvm::ArrayRef<StructField> fields;
};

class FuncType final : public Type {
public:
  FuncType(llvm::ArrayRef<const Type *> argTypes, const Type *returnType)
      : Type(TypeKind::Func), argTypes(argTypes), returnType(returnType) {}

  llvm::ArrayRef<const Type *> getArgTypes() const { return argTypes; }
  [[nodiscard]] const Type *getReturnType() const { return returnType; }

  YUZU_TYPE_RTTI(Func)

private:
  llvm::ArrayRef<const Type *> argTypes;
  const Type *returnType;
};

/// A declared generic type parameter — the `T` in `fn id[T](...)`. A rigid
/// marker, never unified: a call site substitutes a fresh hole for it.
/// `index` is its position in `[...]`; `name` is for diagnostics.
class TypeParamType final : public Type {
public:
  TypeParamType(uint32_t index, std::u32string_view name)
      : Type(TypeKind::TypeParam), name(name), index(index) {}

  [[nodiscard]] uint32_t getIndex() const { return index; }
  [[nodiscard]] std::u32string_view getName() const { return name; }

  YUZU_TYPE_RTTI(TypeParam)

private:
  std::u32string_view name;
  uint32_t index;
};

/// Identifies an inference variable within a `UnificationTable`.
enum class InferId : uint32_t {};

/// An unresolved inference variable, owned by a `UnificationTable`. Its
/// `id` indexes the table's union-find; `flavor` restricts what it can
/// unify with and how it defaults. Replaced by a concrete type during the
/// solve, so it never escapes into a finished tree.
class InferType final : public Type {
public:
  InferType(InferId id, InferKind inferKind)
      : Type(TypeKind::Infer), id(id), inferKind(inferKind) {}

  [[nodiscard]] InferId getId() const { return id; }
  [[nodiscard]] InferKind getInferKind() const { return inferKind; }

  YUZU_TYPE_RTTI(Infer)

private:
  InferId id;
  InferKind inferKind;
};
} // namespace yuzu::types

#endif // YUZU_HIR_TYPES_TYPES_H
