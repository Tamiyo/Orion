// The C++ namespace comes from the unique Base def's `Namespace` field.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "custom::hir::pkg";
  let TreeName = "Hir";
}

def TestExpr : Variant<TestRoot>;
def TestNode : Node<TestExpr>;

// CHECK:      namespace custom::hir::pkg {
// CHECK:      class TestExpr;
// CHECK:      class TestNode;
// CHECK:      } // namespace custom::hir::pkg
