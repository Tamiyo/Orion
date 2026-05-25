// `Children<T>:$f` lowers to a `for` loop that visits every pointer in
// the `ArrayRef` view. The iterator variable is named `child`, so a
// Node carrying both a `Child<T>` and a `Children<T>` keeps the names
// distinct.
//
// RUN: %yuzu-tblgen -gen-hir-visitor-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestStmt : Variant<TestRoot>;

def TestBlock : Node<TestRoot> {
  let Fields = (ins Children<TestStmt>:$stmts);
}

// CHECK-LABEL: void walkTestBlock(const TestBlock *node) {
// CHECK-NEXT:  for (const auto *child : node->getStmts()) {
// CHECK-NEXT:    derived().visit(child);
// CHECK-NEXT:  }
// CHECK-NEXT:  }
