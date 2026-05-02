yuzu_tablegen(YuzuAstExprInc       "yuzu/Ast/Expr.h.inc"       -gen-expr-node-decls)
yuzu_tablegen(YuzuAstSyntaxKindInc "yuzu/Ast/SyntaxKind.h.inc" -gen-syntax-kind-decls)

add_library(YuzuAst Expr.cc)
add_dependencies(YuzuAst YuzuAstExprInc YuzuAstSyntaxKindInc)
target_link_libraries(YuzuAst PUBLIC YuzuSyntax)
