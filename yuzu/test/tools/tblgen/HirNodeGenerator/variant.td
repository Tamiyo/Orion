// A Variant exposes the `isA`/`cast` discrimination pair but keeps its ctor
// `protected` — only concrete `Node`s should construct an `Expr`, and they
// do so by plumbing their own `HirKind` up through it. `cast` returns
// `const T *` (nullptr on mismatch); arena pointers are the natural
// currency on the HIR side.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestExpr : Variant<TestRoot>;
def TestAlpha : Node<TestExpr>;
def TestBeta  : Node<TestExpr>;

// CHECK:       class TestExpr;
// CHECK:       class TestAlpha;
// CHECK:       class TestBeta;

// CHECK-LABEL: class {{.*}}TestExpr : public TestRoot {
// CHECK:       public:
// CHECK:       static bool isA(HirKind kind) {
// CHECK-NEXT:  return kind > HirKind::TESTEXPR_FIRST && kind < HirKind::TESTEXPR_LAST;
// CHECK:       static const TestExpr *cast(const HirNode *node)
// CHECK:       if (!isA(node->getKind())) {
// CHECK-NEXT:    return nullptr;
// CHECK:       return static_cast<const TestExpr *>(node);
// CHECK:       protected:
// CHECK:       TestExpr(HirId id, HirKind kind) : TestRoot(id, kind) {}

// CHECK-LABEL: class {{.*}}TestAlpha : public TestExpr {
// CHECK-LABEL: class {{.*}}TestBeta : public TestExpr {

// CHECK-NOT:   std::optional<
// CHECK-NOT:   SyntaxNode
