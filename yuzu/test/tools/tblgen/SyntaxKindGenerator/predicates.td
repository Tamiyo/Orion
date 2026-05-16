// Mirror of the TokenKindGenerator/predicates.td test: the same
// `is<Category>` predicates now exist on `ast::SyntaxKind`, so AST
// consumers can check token categories without casting through
// `lexer::TokenKind`. Non-token SyntaxKind values fall through to false.
//
// RUN: %yuzu-tblgen -gen-syntax-kind-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TokenBase.td"
include "yuzu/DSL/TreeBase.td"

def TestLex : LexGrammar {
  let Namespace = "test::ast";
}

def TestRoot : Base {
  let Namespace = "test::ast";
}

let IsSymbol = 1 in {
  def TestPlus  : Token<["+"]>;
}

let IsTrivia = 1 in {
  def TestSpace : Regex<[""]>;
}

let IsLiteral = 1 in {
  def TestIntegerLiteral : Regex<[""]>;
}

// Predicates are emitted in the same order as TokenKindGenerator:
// Symbol, Punctuation, Keyword, Literal, Trivia.
// CHECK-LABEL: inline bool isSymbol(SyntaxKind kind)
// CHECK:       case SyntaxKind::TestPlus:
// CHECK:       return true;
// CHECK:       default:
// CHECK:       return false;

// isKeyword has no tokens — only the default arm.
// CHECK-LABEL: inline bool isKeyword(SyntaxKind kind)
// CHECK:       default:
// CHECK:       return false;

// CHECK-LABEL: inline bool isLiteral(SyntaxKind kind)
// CHECK:       case SyntaxKind::TestIntegerLiteral:
// CHECK:       return true;

// CHECK-LABEL: inline bool isTrivia(SyntaxKind kind)
// CHECK:       case SyntaxKind::TestSpace:
// CHECK:       return true;
