// The HIR root carries a `HirId` and `HirKind` directly (no `SyntaxNode`
// backing). It exposes `getId` / `getKind` accessors; the ctor is
// `protected` so the root can't be constructed standalone.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

// CHECK-LABEL: class {{.*}}TestRoot {
// CHECK:       public:
// CHECK:       HirId getId() const { return id; }
// CHECK:       HirKind getKind() const { return kind; }
// CHECK:       protected:
// CHECK:       TestRoot(HirId id, HirKind kind) : id(id), kind(kind) {}
// CHECK:       TestRoot() = delete;
// CHECK:       HirId id;
// CHECK-NEXT:  HirKind kind;

// CHECK-NOT:   SyntaxNode
