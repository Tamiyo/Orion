// The `visitX` hook is the user override point. The generator emits a
// no-op default — the body is `(void)node;` so derived classes that
// don't override stay warning-free.
//
// RUN: %yuzu-tblgen -gen-hir-visitor-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestExpr : Variant<TestRoot>;
def TestAlpha : Node<TestExpr>;

// CHECK: void visitTestAlpha(const TestAlpha *node) { (void)node; }
