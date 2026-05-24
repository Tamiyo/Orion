// `Custom<T>` / `Val<T>` pick by-value vs `const T &` at the API
// boundary from the inner Native's `IsTrivial` bit. Trivial natives
// (small PODs, pointers) stay by value in accessors and ctor params;
// non-trivial ones (here: `TestString`) become `const T &`. Storage is
// always by value either way.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def Id : Native<"yuzu::hir::HirId"> { let IsTrivial = 1; }
def TestInt : Native<"int64_t"> { let IsTrivial = 1; }
def TestString : Native<"std::u32string"> {}

def TestRoot : Base {
  let Namespace = "test::hir";
  let Fields = (ins Custom<Id>:$id);
}

def TestStmt : Variant<TestRoot>;

def TestBinding : Node<TestStmt> {
  let Fields = (ins
    Custom<TestString>:$name,
    Custom<TestInt>:$slot
  );
}

// Trivial fields stay by value; non-trivial wraps in `const T &`. Member
// storage stays by-value either way.
// CHECK-LABEL: class {{.*}}TestBinding : public TestStmt {
// CHECK:       TestBinding(const TestString &name, TestInt slot, Id id)
// CHECK:       : TestStmt(HirKind::TestBinding, id), name(name), slot(slot) {}
// CHECK:       const TestString &getName() const { return name; }
// CHECK:       TestInt getSlot() const { return slot; }
// CHECK:       private:
// CHECK:       TestString name;
// CHECK-NEXT:  TestInt slot;
