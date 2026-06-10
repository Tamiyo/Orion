// Negative: a Field subclass the HIR generator can't lower (Native here)
// must produce a fatal error rather than emit a broken class.
//
// RUN: %not %yuzu-tblgen -gen-hir-node-decls -I %td-include %s 2>&1 | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
}

def TestExpr : Variant<TestRoot>;

def TestBad : Node<TestExpr> {
  let Fields = (ins Native<"int">:$x);
}

// CHECK: error: yuzu-tblgen: HirNodeGenerator has no storage emitter for this Field kind yet
