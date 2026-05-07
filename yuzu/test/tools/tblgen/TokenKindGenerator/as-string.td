// asString takes std::optional<TokenKind>: returns "None" for nullopt,
// otherwise the kind's def name as a string. Includes Error explicitly
// (it's not in the def list — emitted inline in the enum).
//
// RUN: %yuzu-tblgen -gen-token-kind-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TokenBase.td"

def TestLex : LexGrammar {
  let Namespace = "test::lexer";
}

def TestPlus : Token<["+"]>;

// CHECK-LABEL: inline std::string asString(std::optional<TokenKind> kind)
// CHECK:       if (!kind.has_value())
// CHECK-NEXT:    return "None";
// CHECK:       case TokenKind::TestPlus:
// CHECK-NEXT:    return "TestPlus";
// CHECK:       case TokenKind::Error:
// CHECK-NEXT:    return "Error";
