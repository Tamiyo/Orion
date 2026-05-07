// The C++ namespace comes from the unique Base def's `Namespace` field.
//
// RUN: %yuzu-tblgen -gen-ast-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "custom::pkg";
}

def TestExpr : Variant<TestRoot>;
def TestNode : Node<TestExpr>;

// CHECK:      namespace custom::pkg {
// CHECK:      class TestExpr;
// CHECK:      class TestNode;
// CHECK:      } // namespace custom::pkg
