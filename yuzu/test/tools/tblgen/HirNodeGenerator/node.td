// Concrete Node stores its own fields directly and accepts them
// child-first in its ctor, then inherited fields (variant chain →
// Base) appended at the end. The Node forwards `HirKind::<Self>` plus
// inherited args up to the parent; own fields stay in its storage.
// `isA` is an equality check on the concrete kind; `cast` returns a
// downcast pointer or nullptr.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Id : Native<"yuzu::hir::HirId"> {}

def TestOp : Enum<"uint8_t"> {
  let Cases = [EnumCase<"Add">, EnumCase<"Sub">];
}

def TestRoot : Base {
  let Namespace = "test::hir";
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

// CHECK-LABEL: class {{.*}}TestBinary : public TestExpr {
// CHECK:       public:
// CHECK:       TestBinary(const TestExpr *lhs, TestOp op, const TestExpr *rhs, Id id)
// CHECK:       : TestExpr(HirKind::TestBinary, id), lhs(lhs), op(op), rhs(rhs) {}
// CHECK:       static bool isA(HirKind kind) {
// CHECK-NEXT:  return kind == HirKind::TestBinary;
// CHECK:       static const TestBinary *cast(const HirNode *node)
// CHECK:       const TestExpr *getLhs() const { return lhs; }
// CHECK:       TestOp getOp() const { return op; }
// CHECK:       const TestExpr *getRhs() const { return rhs; }
// CHECK:       private:
// CHECK:       const TestExpr *lhs;
// CHECK-NEXT:  TestOp op;
// CHECK-NEXT:  const TestExpr *rhs;
