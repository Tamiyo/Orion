// Variants have `protected` ctors and no `HirKind` of their own, so the
// builder skips them — only concrete `Node`s get a `makeXxx`.
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
}

def TestExpr : Variant<TestRoot>;
def TestLiteral : Node<TestExpr>;

// CHECK:     makeTestLiteral
// CHECK-NOT: makeTestExpr
// CHECK-NOT: makeTestRoot
