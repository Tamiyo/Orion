// `asDisplayString` is the prose-friendly counterpart to `asString`. It
// picks the first non-empty source from:
//   1. the def's explicit `Display`,
//   2. `` `<value>` `` for a single-value `Token`,
//   3. the def name as a last resort.
// The TOKENS_FIRST/TOKENS_LAST sentinels and Error aren't real source
// tokens; calling on them goes to `yuzu_unreachable`.
//
// RUN: %yuzu-tblgen -gen-token-kind-decls -I %td-include %s | %FileCheck %s

include "yuzu/DSL/TokenBase.td"

def TestLex : LexGrammar {
  let Namespace = "test::lexer";
}

// Single-value Token: auto-derived as `<value>`.
def TestPlus : Token<["+"]>;

// Multi-value Token without an override: falls back to the def name.
def TestBool : Token<["true", "false"]>;

// Multi-value Token with override: uses Display verbatim.
def TestKeyword : Token<["true", "false"]> { let Display = "boolean literal"; }

// Regex with override: uses Display verbatim.
def TestIdent : Regex<["[a-zA-Z_][a-zA-Z0-9_]*"]> { let Display = "identifier"; }

// CHECK-LABEL: inline std::string asDisplayString(TokenKind kind)
// CHECK:       case TokenKind::TestPlus:
// CHECK-NEXT:    return "`+`";
// CHECK:       case TokenKind::TestBool:
// CHECK-NEXT:    return "TestBool";
// CHECK:       case TokenKind::TestKeyword:
// CHECK-NEXT:    return "boolean literal";
// CHECK:       case TokenKind::TestIdent:
// CHECK-NEXT:    return "identifier";
// CHECK:       case TokenKind::TOKENS_FIRST:
// CHECK-NEXT:  case TokenKind::TOKENS_LAST:
// CHECK-NEXT:  case TokenKind::Error:
// CHECK-NEXT:    util::yuzu_unreachable();
