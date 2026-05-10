// asString takes a TokenKind directly and returns the kind's def name as a
// string. The TOKENS_FIRST/TOKENS_LAST sentinels and Error are included so
// the switch is exhaustive.
//
// RUN: %yuzu-tblgen -gen-token-kind-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TokenBase.td"

def TestLex : LexGrammar {
  let Namespace = "test::lexer";
}

def TestPlus : Token<["+"]>;

// CHECK-LABEL: inline std::string asString(TokenKind kind)
// CHECK:       case TokenKind::TOKENS_FIRST:
// CHECK-NEXT:    return "TOKENS_FIRST";
// CHECK:       case TokenKind::TestPlus:
// CHECK-NEXT:    return "TestPlus";
// CHECK:       case TokenKind::TOKENS_LAST:
// CHECK-NEXT:    return "TOKENS_LAST";
// CHECK:       case TokenKind::Error:
// CHECK-NEXT:    return "Error";
