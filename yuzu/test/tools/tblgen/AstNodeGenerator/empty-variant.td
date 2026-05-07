// A Variant with no concrete children is no longer a special case under the
// view-struct shape — it's just a normal struct whose isA range covers
// only the variant's own SyntaxKind (between FIRST/LAST sentinels).
//
// RUN: %yuzu-tblgen -gen-ast-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::ast";
}

def TestEmpty : Variant<TestRoot>;

// CHECK-LABEL: class {{.*}}TestRoot {
// CHECK:       protected:
// CHECK:       explicit TestRoot(syntax::SyntaxNode n) : node(std::move(n)) {}
// CHECK:       syntax::SyntaxNode node;

// CHECK:       class TestEmpty;

// CHECK-LABEL: class {{.*}}TestEmpty : public TestRoot {
// CHECK:       public:
// CHECK:       explicit TestEmpty(syntax::SyntaxNode n) : TestRoot(std::move(n)) {}
// CHECK:       static bool isA(SyntaxKind kind)
// CHECK:       return kind > SyntaxKind::TESTEMPTY_FIRST &&
// CHECK-NEXT:  kind < SyntaxKind::TESTEMPTY_LAST;
// CHECK:       };

// CHECK-NOT:   no concrete subtypes
