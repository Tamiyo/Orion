//===----------------------------------------------------------------------===//
// HirKind umbrella.
//===----------------------------------------------------------------------===//
//
// Single TableGen entry point used for `yuzu-tblgen -gen-hir-kind-decls`.
// Pulls in both the AST and lex defs so the SyntaxKind generator can union
// every Variant/Node with every Token/Regex into one enum.
//
//
//===----------------------------------------------------------------------===//

include "yuzu/Hir/Hir.td"
