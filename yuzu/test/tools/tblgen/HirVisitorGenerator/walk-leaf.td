// A leaf Node — no `Child` / `Children` fields, only data — emits an
// empty walker. The body is `(void)node;` so `-Wunused-parameter`
// stays quiet without forcing every Node to declare a child.
//
// RUN: %yuzu-tblgen -gen-hir-visitor-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Int64 : Native<"int64_t"> { let IsTrivial = 1; }

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestExpr : Variant<TestRoot>;

def TestIntLit : Node<TestExpr> {
  let Fields = (ins Custom<Int64>:$value);
}

// CHECK-LABEL: void walkTestIntLit(const TestIntLit *node) { (void)node; }
