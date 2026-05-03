// Negative: a Field subclass the generator doesn't know how to emit
// accessors for (Native here, since only Child is wired up) must produce a
// fatal error rather than a silent miscompile.
//
// RUN: %not %yuzu-tblgen -gen-ast-node-decls -I %td-include %s 2>&1 | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::ast";
}

def TestExpr : Variant<TestRoot>;

def TestBad : Node<TestExpr> {
  let Fields = (ins Native<"int">:$x);
}

// CHECK: error: yuzu-tblgen: AstNodeGenerator has no accessor emitter for this Field kind yet
