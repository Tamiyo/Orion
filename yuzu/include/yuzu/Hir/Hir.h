#ifndef YUZU_HIR_HIR_H
#define YUZU_HIR_HIR_H

#include "yuzu/Util/ErrorHandling.h" // IWYU pragma: keep

#include <llvm/ADT/ArrayRef.h>

#include <cstdint> // IWYU pragma: keep
#include <string>  // IWYU pragma: keep

// Generated HirKind enum + asString. Emits its own
// `namespace yuzu::hir { ... }` block, reopened by the include below.
#include "yuzu/Hir/HirKind.h.inc" // IWYU pragma: export

// Generated HIR class hierarchy (Base + Variants + concrete Nodes). Stores
// fields directly — no SyntaxNode backing — so all child pointers are
// `const T *` into the arena managed by `HirBuilder`.
#include "yuzu/Hir/Hir.h.inc" // IWYU pragma: export

#endif // YUZU_HIR_HIR_H
