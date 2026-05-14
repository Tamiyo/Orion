#ifndef YUZU_HIR_HIR_H
#define YUZU_HIR_HIR_H

#include "yuzu/Util/ErrorHandling.h" // IWYU pragma: keep

#include <llvm/ADT/ArrayRef.h>

#include <cstdint> // IWYU pragma: keep
#include <string>  // IWYU pragma: keep
#include <limits>
#include <type_traits>

namespace yuzu::hir {
/// Stable integer handle for an HIR node. Assigned monotonically by
/// `HirBuilder` and used as the key in side-tables (`BodySourceMap`,
/// future type / name-resolution tables, LSP hover). Strong-typedef'd
/// via empty `enum class` so a `HirId` can't be silently mixed with an
/// unrelated `uint32_t`.
enum class HirId : uint32_t {};

/// Sentinel for nodes with no associated source — synthetic wrappers,
/// pre-builder construction, etc.
inline constexpr HirId InvalidHirId{
    std::numeric_limits<std::underlying_type_t<HirId>>::max()};
} // namespace yuzu::hir

// Generated HirKind enum + asString. Emits its own
// `namespace yuzu::hir { ... }` block, reopened by the include below.
#include "yuzu/Hir/HirKind.h.inc" // IWYU pragma: export

// Generated HIR class hierarchy (Base + Variants + concrete Nodes). Stores
// fields directly — no SyntaxNode backing — so all child pointers are
// `const T *` into the arena managed by `HirBuilder`.
#include "yuzu/Hir/Hir.h.inc" // IWYU pragma: export

#endif // YUZU_HIR_HIR_H
