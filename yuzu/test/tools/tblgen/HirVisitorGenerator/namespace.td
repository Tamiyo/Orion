// The visitor template is emitted into the namespace declared on the
// grammar's Base def, mirroring every other HIR generator.
//
// RUN: %yuzu-tblgen -gen-hir-visitor-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

// CHECK:       namespace test::hir {
// CHECK:       template <typename Derived> class HirVisitor {
// CHECK:       } // namespace test::hir
