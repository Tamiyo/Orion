// `Children<T>:$f` takes an `ArrayRef<const T *>` view from the caller; the
// builder copies it into the arena via `.copy(allocator)` before handing it
// to the node ctor. That way the caller's input array doesn't need to
// outlive the HIR.
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
}

def TestStmt : Variant<TestRoot>;

def TestProgram : Node<TestRoot> {
  let Fields = (ins Children<TestStmt>:$stmts);
}

// CHECK-LABEL: const TestProgram *makeTestProgram(llvm::ArrayRef<const TestStmt *> stmts) {
// CHECK-NEXT:  return new (allocator) TestProgram(allocId(), stmts.copy(allocator));
// CHECK-NEXT:  }
