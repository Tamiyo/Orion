// Each `Native<"...">` def is emitted as a C++ `using` alias so
// `Val<NativeDef>` and `Custom<NativeDef>` accessors can name the
// schema-side type without callers writing the underlying C++ spelling.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestInt64 : Native<"int64_t"> {}
def TestFloat : Native<"double"> {}

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
}

// CHECK-DAG: using TestInt64 = int64_t;
// CHECK-DAG: using TestFloat = double;
