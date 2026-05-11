// A fieldless Node still gets a `makeXxx()` method — it just takes no
// arguments and forwards none to the ctor.
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestExpr : Variant<TestRoot>;
def TestLiteral : Node<TestExpr>;

// CHECK-LABEL: const TestLiteral *makeTestLiteral() { return new (allocator) TestLiteral(); }
