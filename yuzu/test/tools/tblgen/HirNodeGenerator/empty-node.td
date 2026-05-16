// A Node with no own Fields still inherits the Base's `$id`. Ctor takes
// just the inherited `id`, no `private:` storage section since the Node
// itself owns nothing.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Id : Native<"yuzu::hir::HirId"> {}

def TestRoot : Base {
  let Namespace = "test::hir";
  let Fields = (ins Custom<Id>:$id);
}

def TestExpr : Variant<TestRoot>;
def TestLiteral : Node<TestExpr>;

// CHECK-LABEL: class {{.*}}TestLiteral : public TestExpr {
// CHECK:       public:
// CHECK:       TestLiteral(Id id)
// CHECK:       : TestExpr(HirKind::TestLiteral, id) {}
// CHECK:       static bool isA(HirKind kind) {
// CHECK-NEXT:  return kind == HirKind::TestLiteral;
// CHECK:       static const TestLiteral *cast(const HirNode *node)
// CHECK:       };

// CHECK-NOT:   TestLiteral{{.*}}private:
