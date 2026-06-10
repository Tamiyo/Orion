#ifndef YUZU_ANF_ANFVISITOR_H
#define YUZU_ANF_ANFVISITOR_H

#include "yuzu/Anf/Anf.h"            // IWYU pragma: keep
#include "yuzu/Util/ErrorHandling.h" // IWYU pragma: keep

// Generated CRTP `AnfVisitor<Derived>` template. `visit(node)` is the only
// public entry; per-Node `traverseX` / `walkX` / `visitX` hooks are protected.
// Default `traverseX` walks children then visits (post-order); default `walkX`
// visits every schema-declared `Child` / `Children` field; default `visitX` is
// a no-op. Derive `class Foo : public AnfVisitor<Foo>` and override the hook
// matching the granularity you need.
#include "yuzu/Anf/AnfVisitor.h.inc" // IWYU pragma: export

#endif // YUZU_ANF_ANFVISITOR_H
