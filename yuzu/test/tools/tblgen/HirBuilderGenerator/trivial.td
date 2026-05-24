// Builder factory params mirror the node ctor's rule: trivial
// `Custom<T>` / `Val<T>` (inner Native flagged `IsTrivial`) stay by
// value, non-trivial wraps in `const T &`. The inherited `Custom<Id>` is
// auto-supplied via `allocId()` and never appears in the signature.
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

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

// CHECK-LABEL: const TestBinding *makeTestBinding(const TestString &name, TestInt slot) {
// CHECK-NEXT:  return new (allocator) TestBinding(name, slot, allocId());
// CHECK-NEXT:  }
