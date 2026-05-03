// SyntaxKind unions tokens with AST nodes, wrapped in the namespace from the
// AST root Base def. Tokens come first, each Variant's section follows in
// source order, and the System block (Error/Tombstone) closes it.
//
// RUN: %yuzu-tblgen -gen-syntax-kind-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TokenBase.td"
include "yuzu/DSL/TreeBase.td"

def TestLex : LexGrammar {
  let Namespace = "test::lexer";
}

def TestPlus : Token<["+"]>;
def TestNum  : Regex<["[0-9]+"]>;

def TestRoot : Base {
  let Namespace = "test::ast";
}

def TestExpr : Variant<TestRoot>;
def TestBinary : Node<TestExpr>;

// CHECK: namespace test::ast {
// CHECK-LABEL: enum class SyntaxKind
// CHECK:       // Tokens
// CHECK:       TOKENS_FIRST,
// CHECK:       TOKENS_LAST,
// CHECK:       // TestExpr
// CHECK:       TESTEXPR_FIRST,
// CHECK:       TestExpr,
// CHECK:       TestBinary,
// CHECK:       TESTEXPR_LAST,
// CHECK:       // System
// CHECK:       Error,
// CHECK:       Tombstone,
// CHECK:       };
// CHECK:       } // namespace test::ast
