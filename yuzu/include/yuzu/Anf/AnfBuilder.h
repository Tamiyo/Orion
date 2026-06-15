#ifndef YUZU_ANF_ANFBUILDER_H
#define YUZU_ANF_ANFBUILDER_H

#include "yuzu/Anf/Anf.h" // IWYU pragma: export

#include <llvm/Support/Allocator.h>

#include <deque>  // IWYU pragma: keep
#include <new>    // IWYU pragma: keep
#include <vector> // IWYU pragma: keep

// Generated `AnfBuilder` — arena-backed factory with `makeXxx` per concrete
// Node. `Child<T>` parameters are forwarded as-is; a mutable tree's
// `Children<T>` lists are owned by the builder (a `std::deque` of vectors) so
// passes can splice them; an immutable tree copies them into the arena.
#include "yuzu/Anf/AnfBuilder.h.inc" // IWYU pragma: export

#endif // YUZU_ANF_ANFBUILDER_H
