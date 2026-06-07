#ifndef YUZU_HIR_HIRVISITOR_H
#define YUZU_HIR_HIRVISITOR_H

#include "yuzu/Hir/Hir.h"            // IWYU pragma: keep
#include "yuzu/Util/ErrorHandling.h" // IWYU pragma: keep

// Generated CRTP `HirVisitor<Derived>` template. `visit(node)` is the
// only public entry — internal hooks are protected so external code
// can't bypass dispatch by calling `c.traverseFoo(...)` directly. For
// each concrete Node the template carries three overridable hooks:
//
//   traverseX(node)  — default: walk children, then visit. Override to
//                      take full control of the per-node sequencing
//                      (e.g. visit before walking, or skip the walk
//                      conditionally).
//   walkX(node)      — default: invoke `visit` on every schema-declared
//                      Child<T> / Children<T> field. Override to
//                      reorder, filter, or otherwise customise child
//                      traversal.
//   visitX(node)     — default: no-op. Override to do work on the way
//                      back up (post-order by default).
//
// Subclass via CRTP — overrides must be `public` (the base calls them
// through `derived()`, and a `protected` override would fail C++'s
// naming-class access rule):
//
//   class Counter : public yuzu::hir::HirVisitor<Counter> {
//   public:
//     void visitIntLit(const yuzu::hir::IntLit *) { ++n; }
//     std::size_t n = 0;
//   };
//
//   Counter c;
//   c.visit(root); // dispatches by HirKind into the right traverse.
#include "yuzu/Hir/HirVisitor.h.inc" // IWYU pragma: export

#endif // YUZU_HIR_HIRVISITOR_H
