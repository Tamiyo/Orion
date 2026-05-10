// asString takes a SyntaxKind directly and returns the kind's def name as a
// string. Sentinels (TOKENS_FIRST/_LAST, <V>_FIRST/_LAST) and the System
// kinds (Error/Tombstone) are emitted too so the switch is exhaustive.
//
// RUN: %yuzu-tblgen -gen-syntax-kind-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TokenBase.td"
include "yuzu/DSL/TreeBase.td"

def TestLex : LexGrammar {
  let Namespace = "test::lexer";
}

def TestPlus : Token<["+"]>;

def TestRoot : Base {
  let Namespace = "test::ast";
}

def TestExpr : Variant<TestRoot>;
def TestBinary : Node<TestExpr>;

// Base-parented Node: gets its own asString case but no surrounding
// sentinels, since it isn't part of a Variant family.
def TestProgram : Node<TestRoot>;

// CHECK-LABEL: inline std::string asString(SyntaxKind kind)
// CHECK:       case SyntaxKind::TOKENS_FIRST:
// CHECK-NEXT:    return "TOKENS_FIRST";
// CHECK:       case SyntaxKind::TestPlus:
// CHECK-NEXT:    return "TestPlus";
// CHECK:       case SyntaxKind::TOKENS_LAST:
// CHECK-NEXT:    return "TOKENS_LAST";
// CHECK:       case SyntaxKind::TESTEXPR_FIRST:
// CHECK-NEXT:    return "TESTEXPR_FIRST";
// CHECK:       case SyntaxKind::TestExpr:
// CHECK-NEXT:    return "TestExpr";
// CHECK:       case SyntaxKind::TestBinary:
// CHECK-NEXT:    return "TestBinary";
// CHECK:       case SyntaxKind::TESTEXPR_LAST:
// CHECK-NEXT:    return "TESTEXPR_LAST";
// CHECK:       case SyntaxKind::NODES_FIRST:
// CHECK-NEXT:    return "NODES_FIRST";
// CHECK:       case SyntaxKind::TestProgram:
// CHECK-NEXT:    return "TestProgram";
// CHECK:       case SyntaxKind::NODES_LAST:
// CHECK-NEXT:    return "NODES_LAST";
// CHECK:       case SyntaxKind::SYSTEM_FIRST:
// CHECK-NEXT:    return "SYSTEM_FIRST";
// CHECK:       case SyntaxKind::Error:
// CHECK-NEXT:    return "Error";
// CHECK:       case SyntaxKind::Tombstone:
// CHECK-NEXT:    return "Tombstone";
// CHECK:       case SyntaxKind::SYSTEM_LAST:
// CHECK-NEXT:    return "SYSTEM_LAST";
