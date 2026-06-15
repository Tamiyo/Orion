#ifndef YUZU_ANF_ANF_H
#define YUZU_ANF_ANF_H

#include "yuzu/Anf/Ops/Op.h" // IWYU pragma: keep
#include "yuzu/Types/Type.h" // IWYU pragma: keep

#include <llvm/ADT/ArrayRef.h> // IWYU pragma: keep

#include <cstdint>
#include <limits>
#include <type_traits>
#include <vector> // IWYU pragma: keep

namespace yuzu::anf {
/// Stable integer handle for an ANF node. Assigned monotonically by
/// `AnfBuilder`. Strong-typedef'd via empty `enum class` so an `AnfId` can't be
/// silently mixed with a `HirId` or a bare `uint32_t`.
enum class AnfId : uint32_t {};

/// Sentinel for nodes with no associated id — synthetic wrappers, pre-builder
/// construction, etc.
inline constexpr AnfId InvalidAnfId{
    std::numeric_limits<std::underlying_type_t<AnfId>>::max()};

/// Forward-declared so the generated `BindingRef` / `FuncStmtRef` aliases (the
/// `VarAtom` -> def and `FuncRef` -> callee back-edges) resolve; the full
/// definitions land in the generated header below.
class Binding;
class FuncStmt;
} // namespace yuzu::anf

// Generated AnfKind enum + asString. Emits its own `namespace yuzu::anf { ...
// }` block, reopened by the include below.
#include "yuzu/Anf/AnfKind.h.inc" // IWYU pragma: export

// Generated ANF class hierarchy (Base + Variants + concrete Nodes), storing
// fields directly as `const T *` pointers into the arena owned by `AnfBuilder`.
#include "yuzu/Anf/Anf.h.inc" // IWYU pragma: export

#endif // YUZU_ANF_ANF_H
