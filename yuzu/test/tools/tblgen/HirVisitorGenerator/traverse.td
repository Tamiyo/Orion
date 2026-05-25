// The default `traverseX` walks the children, then visits — both calls
// route through `derived()` so an override of either `walkX` or
// `visitX` participates. Overriding `traverseX` itself swaps the whole
// sequence (e.g. pre-order: visit first, then walk).
//
// RUN: %yuzu-tblgen -gen-hir-visitor-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestExpr : Variant<TestRoot>;

def TestBinary : Node<TestExpr> {
  let Fields = (ins
    Child<TestExpr>:$lhs,
    Child<TestExpr>:$rhs
  );
}

// CHECK-LABEL: void traverseTestBinary(const TestBinary *node) {
// CHECK-NEXT:  derived().walkTestBinary(node);
// CHECK-NEXT:  derived().visitTestBinary(node);
// CHECK-NEXT:  }
