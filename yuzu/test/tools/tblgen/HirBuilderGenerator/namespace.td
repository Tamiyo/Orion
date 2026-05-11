// The builder lives in the same namespace as the rest of the HIR — read
// from the unique Base def's `Namespace` field.
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "custom::hir::pkg";
}

// CHECK:      namespace custom::hir::pkg {
// CHECK:      class HirBuilder {
// CHECK:      } // namespace custom::hir::pkg
