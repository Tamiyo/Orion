// A Variant is a POD-shaped view; isA does a SyntaxKind range check
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

// CHECK-LABEL: class {{.*}}TestRoot {
// CHECK:       protected:
// CHECK:       explicit TestRoot(syntax::SyntaxNode n) : node(std::move(n)) {}
// CHECK:       syntax::SyntaxNode node;

// CHECK:       class TestExpr;
// CHECK:       class TestAlpha;
// CHECK:       class TestBeta;

// CHECK-LABEL: class {{.*}}TestExpr : public TestRoot {
// CHECK:       public:
// CHECK:       explicit TestExpr(syntax::SyntaxNode n) : TestRoot(std::move(n)) {}
// CHECK:       static bool isA(SyntaxKind kind)
// CHECK:       return kind > SyntaxKind::TESTEXPR_FIRST &&
// CHECK-NEXT:  kind < SyntaxKind::TESTEXPR_LAST;
// CHECK:       static std::optional<TestExpr> cast(syntax::SyntaxNode n)
// CHECK:       return TestExpr(std::move(n));

// CHECK-LABEL: class {{.*}}TestAlpha : public TestExpr {
// CHECK-LABEL: class {{.*}}TestBeta : public TestExpr {

// CHECK-NOT:   std::variant<
