// A Variant is a POD-shaped view; canCast does a SyntaxKind range check
// against the `<V>_FIRST`/`<V>_LAST` sentinels emitted by SyntaxKindGenerator.
// No `std::variant` — discriminating between concretes is a separate
// `cast()` call to the concrete type.
//
// RUN: %yuzu-tblgen -gen-ast-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::ast";
}

def TestExpr : Variant<TestRoot>;
def TestAlpha : Node<TestExpr>;
def TestBeta  : Node<TestExpr>;

// CHECK:       class TestExpr;
// CHECK:       class TestAlpha;
// CHECK:       class TestBeta;

// CHECK-LABEL: class {{.*}}TestExpr {
// CHECK:       public:
// CHECK:       explicit TestExpr(syntax::SyntaxNode n)
// CHECK:       static bool canCast(SyntaxKind kind)
// CHECK:       return kind > SyntaxKind::TESTEXPR_FIRST &&
// CHECK-NEXT:  kind < SyntaxKind::TESTEXPR_LAST;
// CHECK:       static std::optional<TestExpr> cast(syntax::SyntaxNode n)
// CHECK:       return TestExpr(std::move(n));
// CHECK:       private:
// CHECK:       syntax::SyntaxNode node;

// CHECK-NOT:   std::variant<
