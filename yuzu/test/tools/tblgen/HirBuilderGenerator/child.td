// `Child<T>:$f` lowers to a `const T *` parameter that flows straight into
// the node ctor. The builder placement-news into the arena and returns
// `const T *`.
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

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

// CHECK-LABEL: const TestBinary *makeTestBinary(const TestExpr *lhs, TestOp op,
// CHECK-NEXT:  const TestExpr *rhs) {
// CHECK-NEXT:  return new (allocator) TestBinary(allocId(), lhs, op, rhs);
// CHECK-NEXT:  }
