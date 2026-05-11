// The HIR root carries a `HirKind` directly (no `SyntaxNode` backing). It
// exposes a `getKind()` accessor; the ctor is `protected` so the root can't
// be constructed standalone — concrete `Node`s plumb their kind up
// through it.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

// CHECK-LABEL: class {{.*}}TestRoot {
// CHECK:       public:
// CHECK:       HirKind getKind() const { return kind; }
// CHECK:       protected:
// CHECK:       explicit TestRoot(HirKind kind) : kind(kind) {}
// CHECK:       TestRoot() = delete;
// CHECK:       HirKind kind;

// CHECK-NOT:   SyntaxNode
