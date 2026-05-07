// Two Child<T>:$ slots in the same Node should get sequential `n` values
// when accessed via the same-typed indexing rule (lhs=0, rhs=1). Accessor
// definitions land at file scope so they can see the variant type complete.
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

// CHECK-LABEL: class {{.*}}TestBinary {
// CHECK:       public:
// CHECK:       explicit TestBinary(syntax::SyntaxNode n)
// CHECK:       static bool canCast(SyntaxKind kind)
// CHECK:       return kind == SyntaxKind::TestBinary;
// CHECK:       static std::optional<TestBinary> cast(syntax::SyntaxNode n)
// CHECK:       std::optional<TestExpr> getLhs() const;
// CHECK:       std::optional<TestExpr> getRhs() const;
// CHECK:       private:
// CHECK:       syntax::SyntaxNode node;
// CHECK:       };

// Out-of-line accessor definitions follow.
// CHECK:       inline std::optional<TestExpr> TestBinary::getLhs() const
// CHECK:       return child<TestExpr>(node, 0);
// CHECK:       inline std::optional<TestExpr> TestBinary::getRhs() const
// CHECK:       return child<TestExpr>(node, 1);
