// `Children<T>:$f` lowers to `llvm::ArrayRef<const T *>` storage and an
// accessor returning the same. The HIR builder allocates the underlying
// array in its bump allocator; nodes hold a view over it.
//
// A `Node` parented at the Base (no Variant in between) is the typical
// shape of an HIR root.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestStmt : Variant<TestRoot>;

def TestProgram : Node<TestRoot> {
  let Fields = (ins Children<TestStmt>:$stmts);
}

// CHECK-LABEL: class {{.*}}TestProgram : public TestRoot {
// CHECK:       public:
// CHECK:       TestProgram(llvm::ArrayRef<const TestStmt *> stmts)
// CHECK:       : TestRoot(HirKind::TestProgram), stmts(stmts) {}
// CHECK:       llvm::ArrayRef<const TestStmt *> getStmts() const {
// CHECK-NEXT:  return stmts;
// CHECK:       private:
// CHECK:       llvm::ArrayRef<const TestStmt *> stmts;
