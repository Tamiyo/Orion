//===----------------------------------------------------------------------===//
// SyntaxKind umbrella.
//===----------------------------------------------------------------------===//
//
// Single TableGen entry point used for `yuzu-tblgen -gen-syntax-kind-decls`.
// Pulls in both the AST and lex defs so the SyntaxKind generator can union
// every Variant/Node with every Token/Regex into one enum.
//
// The enum itself is emitted into `SyntaxKind.h.inc` and lands in the AST
// namespace (driven by `AstNode.Namespace` in Ast.td). For per-section
// layout (TOKENS_FIRST/LAST, <VARIANT>_FIRST/LAST, system kinds) see
// `tools/tblgen/SyntaxKindGenerator.cc`.
//
//===----------------------------------------------------------------------===//

include "yuzu/Ast/Ast.td"
include "yuzu/Lexer/TokenKind.td"
