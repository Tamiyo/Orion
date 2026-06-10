// `Child<T>:$f` lowers to a `const T *` parameter that flows straight into
// the node ctor. The builder omits any `Custom<Id>` inherited field from
// its public signature and supplies `allocId()` in its place at the call
// site — own fields first, then inherited (`id` last).
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Id : Native<"yuzu::hir::HirId"> { let IsTrivial = 1; }

def TestOp : Enum<"uint8_t"> {
  let Cases = [EnumCase<"Add">, EnumCase<"Sub">];
}

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
  let Fields = (ins Custom<Id>:$id);
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
// CHECK-NEXT:  return new (allocator) TestBinary(lhs, op, rhs, allocId());
// CHECK-NEXT:  }
