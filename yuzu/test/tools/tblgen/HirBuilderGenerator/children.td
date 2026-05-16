// `Children<T>:$f` takes an `ArrayRef<const T *>` view from the caller; the
// builder copies it into the arena via `.copy(allocator)` before handing it
// to the node ctor. The Base's inherited `Custom<Id>` is auto-supplied
// via `allocId()` at the end of the ctor call.
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Id : Native<"yuzu::hir::HirId"> {}

def TestRoot : Base {
  let Namespace = "test::hir";
  let Fields = (ins Custom<Id>:$id);
}

def TestStmt : Variant<TestRoot>;

def TestProgram : Node<TestRoot> {
  let Fields = (ins Children<TestStmt>:$stmts);
}

// CHECK-LABEL: const TestProgram *makeTestProgram(llvm::ArrayRef<const TestStmt *> stmts) {
// CHECK-NEXT:  return new (allocator) TestProgram(stmts.copy(allocator), allocId());
// CHECK-NEXT:  }
