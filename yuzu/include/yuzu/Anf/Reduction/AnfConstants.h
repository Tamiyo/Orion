#ifndef YUZU_ANF_REDUCTION_ANFCONSTANTS_H
#define YUZU_ANF_REDUCTION_ANFCONSTANTS_H

#include "yuzu/Anf/Anf.h"

#include <cstdint>
#include <string_view>

// Predicates and accessors over constant atoms, for passes that branch on what
// an operand actually is (constant folding, emission). `isX` is null-safe — a
// non-constant atom (a `VarAtom`, a `FieldAtom`) is simply "not an X". `asX`
// reads the value and requires the matching `isX` to hold.

namespace yuzu::anf {

[[nodiscard]] inline bool isInt(const Atom *atom) {
  return IntConst::cast(atom) != nullptr;
}

[[nodiscard]] inline bool isFloat(const Atom *atom) {
  return FloatConst::cast(atom) != nullptr;
}

[[nodiscard]] inline bool isBool(const Atom *atom) {
  return BoolConst::cast(atom) != nullptr;
}

[[nodiscard]] inline bool isString(const Atom *atom) {
  return StringConst::cast(atom) != nullptr;
}

/// Precondition: `isInt(atom)`.
[[nodiscard]] inline int64_t asInt(const Atom *atom) {
  return IntConst::cast(atom)->getValue();
}

/// Precondition: `isFloat(atom)`.
[[nodiscard]] inline double asFloat(const Atom *atom) {
  return FloatConst::cast(atom)->getValue();
}

/// Precondition: `isBool(atom)`.
[[nodiscard]] inline bool asBool(const Atom *atom) {
  return BoolConst::cast(atom)->getValue();
}

/// Precondition: `isString(atom)`.
[[nodiscard]] inline std::u32string_view asString(const Atom *atom) {
  return StringConst::cast(atom)->getValue();
}

} // namespace yuzu::anf

#endif // YUZU_ANF_REDUCTION_ANFCONSTANTS_H
