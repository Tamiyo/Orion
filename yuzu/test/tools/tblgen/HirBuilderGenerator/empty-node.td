// A Node with no own Fields produces a `makeXxx()` factory with no
// parameters — the inherited `Custom<Id>` is auto-supplied via
// `allocId()` and not exposed to the caller.
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Id : Native<"yuzu::hir::HirId"> { let IsTrivial = 1; }

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
  let Fields = (ins Custom<Id>:$id);
}

def TestExpr : Variant<TestRoot>;
def TestLiteral : Node<TestExpr>;

// CHECK-LABEL: const TestLiteral *makeTestLiteral() {
// CHECK-NEXT:  return new (allocator) TestLiteral(allocId());
// CHECK-NEXT:  }
