// Negative: no LexGrammar def in scope means the generator can't pick a
// namespace; it must fail loudly rather than emit a degenerate enum.
//
// RUN: %not %yuzu-tblgen -gen-token-kind-decls -I %td-include %s 2>&1 | %FileCheck %s

include "yuzu/DSL/TokenBase.td"

def TestPlus : Token<["+"]>;

// CHECK: error: yuzu-tblgen: no LexGrammar def declares a non-empty Namespace
