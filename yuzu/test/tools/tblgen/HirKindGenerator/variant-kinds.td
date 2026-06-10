// Each variant gets a narrow `<V>Kind` enum listing its DIRECT children
// (concrete Nodes by their own name, sub-Variants by the variant's
// name), plus `to<V>Kind(HirKind)` and `asString(<V>Kind)`. Callers
// dispatch one level at a time and `-Wswitch` enforces exhaustiveness.
//
// RUN: %yuzu-tblgen -gen-hir-kind-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
}

def TestExpr : Variant<TestRoot>;
def TestBinary : Node<TestExpr>;

def TestLit : Variant<TestExpr>;
def TestIntLit : Node<TestLit>;
def TestFloatLit : Node<TestLit>;

// TestExprKind lists direct children of TestExpr: the concrete
// TestBinary and the sub-variant TestLit (NOT TestLit's leaves).
// CHECK-LABEL: enum class TestExprKind
// CHECK:       TestBinary,
// CHECK-NEXT:  TestLit,
// CHECK:       };

// toTestExprKind maps every transitive concrete leaf to its DIRECT
// ancestor under TestExpr. TestBinary maps to itself; TestIntLit and
// TestFloatLit collapse into TestExprKind::TestLit.
// CHECK-LABEL: inline TestExprKind toTestExprKind(HirKind kind)
// CHECK:       case HirKind::TestBinary:
// CHECK-NEXT:  return TestExprKind::TestBinary;
// CHECK:       case HirKind::TestIntLit:
// CHECK-NEXT:  case HirKind::TestFloatLit:
// CHECK-NEXT:  return TestExprKind::TestLit;
// CHECK:       default:
// CHECK:       util::yuzu_unreachable();

// asString(TestExprKind) exhausts the per-variant enum without a
// default — adding a leaf without regenerating breaks the switch.
// CHECK-LABEL: inline std::string asString(TestExprKind kind)
// CHECK:       case TestExprKind::TestBinary:
// CHECK-NEXT:  return "TestBinary";
// CHECK:       case TestExprKind::TestLit:
// CHECK-NEXT:  return "TestLit";

// The nested TestLit variant gets its own kind enum with just its
// direct concrete leaves.
// CHECK-LABEL: enum class TestLitKind
// CHECK:       TestIntLit,
// CHECK-NEXT:  TestFloatLit,
// CHECK:       };
