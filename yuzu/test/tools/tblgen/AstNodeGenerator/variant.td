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
// CHECK:       public:
// CHECK:       SyntaxKind getKind() const { return node.getKind(); }
// CHECK:       const SyntaxNode &getSyntax() const { return node; }
// CHECK:       lexer::Range getRange() const {
// CHECK-NEXT:  const auto start = static_cast<uint32_t>(node.getOffset());
// CHECK-NEXT:  const auto end = start + static_cast<uint32_t>(node.getGreen().getWidth());
// CHECK-NEXT:  return lexer::Range{start, end};
// CHECK:       protected:
// CHECK:       explicit TestRoot(SyntaxNode node) : node(std::move(node)) {}
// CHECK:       SyntaxNode node;

// CHECK:       class TestExpr;
// CHECK:       class TestAlpha;
// CHECK:       class TestBeta;

// CHECK-LABEL: class {{.*}}TestExpr : public TestRoot {
// CHECK:       public:
// CHECK:       explicit TestExpr(SyntaxNode node) : TestRoot(std::move(node)) {}
// CHECK:       static bool isA(SyntaxKind kind)
// CHECK:       return kind > SyntaxKind::TESTEXPR_FIRST &&
// CHECK-NEXT:  kind < SyntaxKind::TESTEXPR_LAST;
// CHECK:       static std::optional<TestExpr> cast(const TestRoot &node)
// CHECK:       return TestExpr(node.getSyntax());

// CHECK-LABEL: class {{.*}}TestAlpha : public TestExpr {
// CHECK-LABEL: class {{.*}}TestBeta : public TestExpr {

// CHECK-NOT:   std::variant<
