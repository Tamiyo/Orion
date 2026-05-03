// The TokenKind enum is wrapped in the namespace declared by the LexGrammar
// def, sized to the smallest unsigned int that fits, sorted alphabetically,
// and terminated by `Error`.
//
// RUN: %yuzu-tblgen -gen-token-kind-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TokenBase.td"

def TestLex : LexGrammar {
  let Namespace = "test::lexer";
}

let IsSymbol = 1 in {
  def TestPlus  : Token<["+"]>;
  def TestMinus : Token<["-"]>;
}

let IsKeyword = 1 in {
  def TestLet : Token<["let"]>;
}

// CHECK: namespace test::lexer {
// CHECK-LABEL: enum class TokenKind : uint8_t
// CHECK:       TestLet,
// CHECK-NEXT:  TestMinus,
// CHECK-NEXT:  TestPlus,
// CHECK-NEXT:  Error,
// CHECK:       };
// CHECK:       } // namespace test::lexer
