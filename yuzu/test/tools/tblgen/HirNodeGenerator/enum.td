// Each `Enum` def is emitted as a C++ `enum class` before the class
// definitions, so `Custom<EnumDef>:$f` accessors can name it.
//
// RUN: %yuzu-tblgen -gen-hir-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestOp : Enum<"uint8_t"> {
  let Cases = [
    EnumCase<"Add">,
    EnumCase<"Sub">,
    EnumCase<"Mul">
  ];
}

def TestRoot : Base {
  let Namespace = "test::hir";
}

// CHECK-LABEL: enum class {{.*}}TestOp : uint8_t {
// CHECK-NEXT:  Add,
// CHECK-NEXT:  Sub,
// CHECK-NEXT:  Mul,
// CHECK-NEXT:  };

// CHECK-LABEL: inline std::string asString(TestOp value)
// CHECK:       case TestOp::Add:
// CHECK-NEXT:    return "Add";
// CHECK:       case TestOp::Sub:
// CHECK-NEXT:    return "Sub";
// CHECK:       case TestOp::Mul:
// CHECK-NEXT:    return "Mul";
// CHECK:       util::yuzu_unreachable();
