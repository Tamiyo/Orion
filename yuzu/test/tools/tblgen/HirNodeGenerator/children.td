// `Children<T>:$f` lowers to `llvm::ArrayRef<const T *>` storage and an
// accessor returning the same. Ctor takes the list child-first; inherited
// fields from the Base chain (here: `id`) come at the end.
//
// A `Node` parented at the Base (no Variant in between) is the typical
// shape of an HIR root.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Id : Native<"yuzu::hir::HirId"> { let IsTrivial = 1; }

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
  let Fields = (ins Custom<Id>:$id);
}

def TestStmt : Variant<TestRoot>;

def TestProgram : Node<TestRoot> {
  let Fields = (ins Children<TestStmt>:$stmts);
}

// CHECK-LABEL: class {{.*}}TestProgram : public TestRoot {
// CHECK:       public:
// CHECK:       TestProgram(llvm::ArrayRef<const TestStmt *> stmts, Id id)
// CHECK:       : TestRoot(HirKind::TestProgram, id), stmts(stmts) {}
// CHECK:       llvm::ArrayRef<const TestStmt *> getStmts() const {
// CHECK-NEXT:  return stmts;
// CHECK:       private:
// CHECK:       llvm::ArrayRef<const TestStmt *> stmts;
