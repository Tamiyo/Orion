// Negative: no Base def declares a Namespace, so the generator has no
// namespace to wrap the output in. Must error rather than emit unwrapped
// declarations.
//
// RUN: %not %yuzu-tblgen -gen-ast-node-decls -I %td-include %s 2>&1 | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base;

def TestExpr : Variant<TestRoot>;

// CHECK: error: yuzu-tblgen: no Base def declares a non-empty Namespace
