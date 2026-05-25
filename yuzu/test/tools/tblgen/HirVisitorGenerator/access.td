// `visit(const HirNode *)` is the only public entry; every per-Node
// hook (`traverseX` / `walkX` / `visitX`) sits behind `protected:` so
// external callers can't bypass dispatch by invoking them directly.
// The `derived()` shim stays `private:` — purely an implementation
// detail of the CRTP self-cast.
//
// RUN: %yuzu-tblgen -gen-hir-visitor-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestExpr : Variant<TestRoot>;
def TestAlpha : Node<TestExpr>;

// CHECK:       public:
// CHECK-NEXT:  void visit(const HirNode *node)

// CHECK:       protected:
// CHECK:       void traverseTestAlpha(const TestAlpha *node)
// CHECK:       void walkTestAlpha(const TestAlpha *node)
// CHECK:       void visitTestAlpha(const TestAlpha *node)

// CHECK:       private:
// CHECK-NEXT:  Derived &derived()
