// The builder is a non-copyable wrapper around a `BumpPtrAllocator`.
// Copying would silently duplicate the arena, so we delete it explicitly.
//
// RUN: %yuzu-tblgen -gen-hir-builder-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestRoot : Base {
  let Namespace = "test::hir";
  let TreeName = "Hir";
}

// CHECK-LABEL: class HirBuilder {
// CHECK:       public:
// CHECK:       HirBuilder() = default;
// CHECK:       HirBuilder(const HirBuilder &) = delete;
// CHECK:       HirBuilder &operator=(const HirBuilder &) = delete;
// CHECK:       private:
// CHECK:       HirId allocId() { return HirId{nextId++}; }
// CHECK:       uint32_t nextId = 0;
// CHECK-NEXT:  llvm::BumpPtrAllocator allocator;
// CHECK:       };
