// Negative: a Field subclass the HIR builder can't lower (Native here) must
// produce a fatal error rather than emit a broken makeXxx.
//
// RUN: %not %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s 2>&1 | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestExpr : Variant<TestRoot>;

def TestBad : Node<TestExpr> {
  let Fields = (ins Native<"int">:$x);
}

// CHECK: error: yuzu-tblgen: HirBuilderGenerator has no parameter emitter for this Field kind yet
