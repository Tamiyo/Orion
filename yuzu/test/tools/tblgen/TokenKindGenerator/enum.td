// The TokenKind enum is wrapped in the namespace declared by the LexGrammar
// def, sized to the smallest unsigned int that fits, sorted by source
// location, and bracketed by TOKENS_FIRST/TOKENS_LAST sentinels matching the
// layout SyntaxKindGenerator emits for the token block of SyntaxKind.
// `Error` follows the closing sentinel as the lexer's recovery kind.
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
// CHECK:       TOKENS_FIRST,
// CHECK-NEXT:  TestPlus,
// CHECK-NEXT:  TestMinus,
// CHECK-NEXT:  TestLet,
// CHECK-NEXT:  TOKENS_LAST,
// CHECK-NEXT:  Error,
// CHECK:       };
// CHECK:       } // namespace test::lexer
