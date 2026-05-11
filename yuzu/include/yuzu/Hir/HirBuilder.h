#ifndef YUZU_HIR_HIR_BUILDER_H
#define YUZU_HIR_HIR_BUILDER_H

#include "yuzu/Hir/Hir.h" // IWYU pragma: export

#include <llvm/Support/Allocator.h>

#include <new> // IWYU pragma: keep

// Generated `HirBuilder` — arena-backed factory with `makeXxx` per concrete
// Node. `Child<T>` parameters are forwarded as-is; `Children<T>` views are
// copied into the arena before being handed to the node ctor.
#include "yuzu/Hir/HirBuilder.h.inc" // IWYU pragma: export

#endif // YUZU_HIR_HIR_BUILDER_H
