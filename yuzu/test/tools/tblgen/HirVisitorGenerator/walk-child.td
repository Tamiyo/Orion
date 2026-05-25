// `Child<T>:$f` lowers to a single `derived().visit(node->getF())` line
// in `walkX`. Multiple `Child<T>` fields are visited in declaration
// order. Non-structural fields (`Custom`, `Val`, `Native`, `Enum`) sit
// alongside children in the dag without contributing to the walk —
// they're data for `visitX` to inspect.
//
// RUN: %yuzu-tblgen -gen-hir-visitor-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestOp : Enum<"uint8_t"> {
  let Cases = [EnumCase<"Add">, EnumCase<"Sub">];
}

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestExpr : Variant<TestRoot>;

def TestBinary : Node<TestExpr> {
  let Fields = (ins
    Child<TestExpr>:$lhs,
    Custom<TestOp>:$op,
    Child<TestExpr>:$rhs
  );
}

// CHECK-LABEL: void walkTestBinary(const TestBinary *node) {
// CHECK-NEXT:  derived().visit(node->getLhs());
// CHECK-NEXT:  derived().visit(node->getRhs());
// CHECK-NEXT:  }

// `op` is data, not a tree edge — the walker shouldn't touch it.
// CHECK-NOT:   node->getOp()
