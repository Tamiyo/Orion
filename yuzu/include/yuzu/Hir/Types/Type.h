#ifndef YUZU_HIR_TYPES_TYPES_H
#define YUZU_HIR_TYPES_TYPES_H

#include "yuzu/Util/ErrorHandling.h"

#include <cstdint>
#include <string>

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
  //   Unit,
  //   List,
  //   Tuple,
  //   Func,
  //   Struct,
  //   Class,
  //   Infer,
  Error,
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
} // namespace yuzu::hir

#endif // YUZU_HIR_TYPES_TYPES_H
