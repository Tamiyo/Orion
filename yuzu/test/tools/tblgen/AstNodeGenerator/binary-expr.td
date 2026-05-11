// Two Child<T>:$ slots in the same Node should get sequential `n` values
// when accessed via the same-typed indexing rule (lhs=0, rhs=1). Accessor
// definitions are emitted inline within the class body — every Variant
// they reference is complete by the time the Node is parsed.
//
// RUN: %yuzu-tblgen -gen-ast-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::ast";
}

def TestExpr : Variant<TestRoot>;

def TestBinary : Node<TestExpr> {
  let Fields = (ins Child<TestExpr>:$lhs, Child<TestExpr>:$rhs);
}

// Forward decls and class shape.
// CHECK:       class TestExpr;
// CHECK:       class TestBinary;

// CHECK-LABEL: class {{.*}}TestBinary : public TestExpr {
// CHECK:       public:
// CHECK:       explicit TestBinary(SyntaxNode node) : TestExpr(std::move(node)) {}
// CHECK:       static bool isA(SyntaxKind kind)
// CHECK:       return kind == SyntaxKind::TestBinary;
// CHECK:       static std::optional<TestBinary> cast(const TestRoot &node)
// CHECK:       std::optional<TestExpr> getLhs() const {
// CHECK-NEXT:  return child<TestExpr>(node, 0);
// CHECK:       std::optional<TestExpr> getRhs() const {
// CHECK-NEXT:  return child<TestExpr>(node, 1);
// CHECK:       };

// CHECK-NOT:   TestBinary::getLhs
// CHECK-NOT:   TestBinary::getRhs
