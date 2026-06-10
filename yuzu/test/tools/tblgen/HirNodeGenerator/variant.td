// A Variant exposes `isA`/`cast` and keeps its ctor `protected` — only
// concrete `Node`s should construct it, plumbing `HirKind` + inherited
// fields up through it. Inherited fields follow the variant chain, so a
// Variant with no own Fields just forwards `(kind, ...inherited)`.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Id : Native<"yuzu::hir::HirId"> { let IsTrivial = 1; }

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
  let Fields = (ins Custom<Id>:$id);
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
// CHECK:       TestExpr(HirKind kind, Id id)
// CHECK:       : TestRoot(kind, id) {}

// CHECK-LABEL: class {{.*}}TestAlpha : public TestExpr {
// CHECK-LABEL: class {{.*}}TestBeta : public TestExpr {

// CHECK-NOT:   std::optional<
// CHECK-NOT:   SyntaxNode
