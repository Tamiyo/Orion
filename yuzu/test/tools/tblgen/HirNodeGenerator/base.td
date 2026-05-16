// The HIR root drives its fields from the schema — declare a `Custom<Id>`
// field on the Base def and the generator threads it through every
// constructor. `HirKind` is always implicit (each concrete Node pins it
// via its parent ctor). The Base ctor is `protected` so the root can't be
// constructed standalone.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Id : Native<"yuzu::hir::HirId"> {}

def TestRoot : Base {
  let Namespace = "test::hir";
  let Fields = (ins Custom<Id>:$id);
}

// CHECK-LABEL: class {{.*}}TestRoot {
// CHECK:       public:
// CHECK:       HirKind getKind() const { return kind; }
// CHECK:       Id getId() const { return id; }
// CHECK:       protected:
// CHECK:       TestRoot(HirKind kind, Id id)
// CHECK:       : kind(kind), id(id) {}
// CHECK:       TestRoot() = delete;
// CHECK:       HirKind kind;
// CHECK-NEXT:  Id id;

// CHECK-NOT:   SyntaxNode
