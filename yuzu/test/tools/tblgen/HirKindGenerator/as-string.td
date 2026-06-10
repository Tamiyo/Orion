// asString(HirKind) maps each enumerator to its name. Sentinels and the
// System block are included so the switch is exhaustive (no
// `default:` arm, no -Wswitch warning).
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

// CHECK-LABEL: inline std::string asString(HirKind kind)
// CHECK:       case HirKind::TESTEXPR_FIRST:
// CHECK-NEXT:    return "TESTEXPR_FIRST";
// CHECK:       case HirKind::TestExpr:
// CHECK-NEXT:    return "TestExpr";
// CHECK:       case HirKind::TestBinary:
// CHECK-NEXT:    return "TestBinary";
// CHECK:       case HirKind::TESTEXPR_LAST:
// CHECK-NEXT:    return "TESTEXPR_LAST";
// CHECK:       case HirKind::NODES_FIRST:
// CHECK-NEXT:    return "NODES_FIRST";
// CHECK:       case HirKind::TestProgram:
// CHECK-NEXT:    return "TestProgram";
// CHECK:       case HirKind::NODES_LAST:
// CHECK-NEXT:    return "NODES_LAST";
// CHECK:       case HirKind::SYSTEM_FIRST:
// CHECK-NEXT:    return "SYSTEM_FIRST";
// CHECK:       case HirKind::Error:
// CHECK-NEXT:    return "Error";
// CHECK:       case HirKind::Tombstone:
// CHECK-NEXT:    return "Tombstone";
// CHECK:       case HirKind::SYSTEM_LAST:
// CHECK-NEXT:    return "SYSTEM_LAST";
