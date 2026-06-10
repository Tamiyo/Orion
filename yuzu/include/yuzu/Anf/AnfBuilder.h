#ifndef YUZU_ANF_ANFBUILDER_H
#define YUZU_ANF_ANFBUILDER_H

#include "yuzu/Anf/Anf.h" // IWYU pragma: export

#include <llvm/Support/Allocator.h>

#include <new> // IWYU pragma: keep

// Generated `AnfBuilder` — arena-backed factory with `makeXxx` per concrete
// Node. `Child<T>` parameters are forwarded as-is; `Children<T>` views are
// copied into the arena before being handed to the node ctor.
#include "yuzu/Anf/AnfBuilder.h.inc" // IWYU pragma: export

#endif // YUZU_ANF_ANFBUILDER_H
