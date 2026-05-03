// Two Child<T>:$ slots in the same Node should get sequential `n` values
// when accessed via the same-typed indexing rule (lhs=0, rhs=1). Accessors
// reach the inherited `node` member via `this->` for two-phase lookup.
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

// CHECK: namespace test::ast
// CHECK-LABEL: class TestBinary final : public AstNode<TestBinary>
// CHECK:       explicit TestBinary(syntax::SyntaxNode node)
// CHECK:       TestBinary() = delete;
// CHECK:       static bool canCast(SyntaxKind kind)
// CHECK:       return kind == SyntaxKind::TestBinary;
// CHECK:       static std::optional<TestBinary> cast(syntax::SyntaxNode node)
// CHECK:       std::unique_ptr<TestExpr> getLhs() const
// CHECK:       return child<TestExpr>(this->node, 0);
// CHECK:       std::unique_ptr<TestExpr> getRhs() const
// CHECK:       return child<TestExpr>(this->node, 1);
