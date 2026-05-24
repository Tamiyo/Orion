// A `Native` with a non-empty `ViewName` opts into a non-owning companion
// type. `emitNatives` emits a parallel `{Name}View` alias, and AST
// accessors over `Custom<NativeWithView>` / `Val<NativeWithView>` use
// that view alias inside `std::optional` — callers don't pay for
// materialising an owned value out of the syntax tree.
//
// Natives without a `ViewName` keep emitting `std::optional<T>` over the
// owned alias (here: `TestBool`).
//
// RUN: %yuzu-tblgen -gen-ast-node-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TreeBase.td"

def TestString : Native<"std::u32string"> {
  let ViewName = "std::u32string_view";
}
def TestBool : Native<"bool"> { let IsTrivial = 1; }

def TestRoot : Base {
  let Namespace = "test::ast";
}

def TestExpr : Variant<TestRoot>;

def TestLet : Node<TestExpr> {
  let Fields = (ins
    Custom<TestString>:$name,
    Custom<TestBool>:$mutable
  );
}

// CHECK-DAG:   using TestString = std::u32string;
// CHECK-DAG:   using TestStringView = std::u32string_view;
// CHECK-DAG:   using TestBool = bool;

// View-bearing native uses the view alias; non-view native uses the
// type alias directly.
// CHECK-LABEL: class {{.*}}TestLet : public TestExpr {
// CHECK:       std::optional<TestStringView> getName() const;
// CHECK:       std::optional<TestBool> getMutable() const;
