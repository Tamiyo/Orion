// A fieldless Node still gets an explicit default ctor so the
// `HirKind::<Name>` initializer fires, and skips the `private:` storage
// section entirely.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestExpr : Variant<TestRoot>;
def TestLiteral : Node<TestExpr>;

// CHECK-LABEL: class {{.*}}TestLiteral : public TestExpr {
// CHECK:       public:
// CHECK:       explicit TestLiteral(HirId id) : TestExpr(id, HirKind::TestLiteral) {}
// CHECK:       static bool isA(HirKind kind) {
// CHECK-NEXT:  return kind == HirKind::TestLiteral;
// CHECK:       static const TestLiteral *cast(const HirNode *node)
// CHECK:       };

// CHECK-NOT:   TestLiteral{{.*}}private:
