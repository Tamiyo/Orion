// HirKind is the LLVM-style RTTI discriminator for HIR nodes. Same
// shape as the SyntaxKind generator's variant block (FIRST/LAST
// sentinels around each Variant's children) plus a `// Nodes` block
// for Base-parented Nodes and a `// System` block for Error/Tombstone.
// Unlike SyntaxKind, HirKind has no tokens block — the HIR has no
// lexer concept.
//
// RUN: %yuzu-tblgen -gen-hir-kind-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
}

def TestExpr : Variant<TestRoot>;
def TestBinary : Node<TestExpr>;

def TestProgram : Node<TestRoot>;

// CHECK: namespace test::hir {
// CHECK-LABEL: enum class HirKind
// CHECK:       // TestExpr
// CHECK-NEXT:  TESTEXPR_FIRST,
// CHECK-NEXT:  TestExpr,
// CHECK-NEXT:  TestBinary,
// CHECK-NEXT:  TESTEXPR_LAST,
// CHECK:       // Nodes
// CHECK-NEXT:  NODES_FIRST,
// CHECK-NEXT:  TestProgram,
// CHECK-NEXT:  NODES_LAST,
// CHECK:       // System
// CHECK-NEXT:  SYSTEM_FIRST,
// CHECK-NEXT:  Error,
// CHECK-NEXT:  Tombstone,
// CHECK-NEXT:  SYSTEM_LAST,
// CHECK:       };
// CHECK:       } // namespace test::hir
