// A Variant with no concrete children would be `std::variant<>` which is
// ill-formed. The generator instead emits a placeholder empty class so the
// forward declaration still resolves to a real type.
//
// RUN: %yuzu-tblgen -gen-ast-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::ast";
}

def TestEmpty : Variant<TestRoot>;

// CHECK:       class TestEmpty;
// CHECK:       (no concrete subtypes yet)
// CHECK:       class TestEmpty {};

// And no `std::variant` should appear for TestEmpty.
// CHECK-NOT:   std::variant<>
