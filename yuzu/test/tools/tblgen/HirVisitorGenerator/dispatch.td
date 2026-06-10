// The public `visit(const HirNode *)` entry is a switch over `HirKind`
// for every concrete Node — each case forwards to the corresponding
// `traverseX` through `derived()` so subclasses can override the
// dispatch target. Variant / system kinds never appear on a concrete
// node, so the `default:` arm is `yuzu_unreachable`.
//
// RUN: %yuzu-tblgen -gen-hir-visitor-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
}

def TestExpr : Variant<TestRoot>;
def TestAlpha : Node<TestExpr>;
def TestBeta  : Node<TestExpr>;

// CHECK-LABEL: void visit(const HirNode *node) {
// CHECK-NEXT:  switch (node->getKind()) {
// CHECK-NEXT:  case HirKind::TestAlpha:
// CHECK-NEXT:    return derived().traverseTestAlpha(TestAlpha::cast(node));
// CHECK-NEXT:  case HirKind::TestBeta:
// CHECK-NEXT:    return derived().traverseTestBeta(TestBeta::cast(node));
// CHECK-NEXT:  default:
// CHECK-NEXT:    util::yuzu_unreachable();
// CHECK-NEXT:  }
// CHECK-NEXT:  }
