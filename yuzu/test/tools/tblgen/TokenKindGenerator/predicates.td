// `is<Category>` predicates emit a switch over each token in that category,
// returning true on a match and false in the default arm. Empty categories
// emit just the default arm.
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

// CHECK-LABEL: inline bool isSymbol(TokenKind kind)
// CHECK:       case TokenKind::TestMinus:
// CHECK-NEXT:  case TokenKind::TestPlus:
// CHECK:       return true;
// CHECK:       default:
// CHECK:       return false;

// CHECK-LABEL: inline bool isKeyword(TokenKind kind)
// CHECK:       case TokenKind::TestLet:
// CHECK:       return true;

// isLiteral has no tokens — only the default arm.
// CHECK-LABEL: inline bool isLiteral(TokenKind kind)
// CHECK:       default:
// CHECK:       return false;
