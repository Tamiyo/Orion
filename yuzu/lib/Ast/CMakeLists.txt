yuzu_tablegen(YuzuAstInc           "yuzu/Ast/Ast.h.inc"        -gen-ast-node-decls    "yuzu/Ast/Ast.td")
yuzu_tablegen(YuzuAstSyntaxKindInc "yuzu/Ast/SyntaxKind.h.inc" -gen-syntax-kind-decls "yuzu/Ast/SyntaxKind.td")

add_library(YuzuAst Expr.cc)
add_dependencies(YuzuAst YuzuAstInc YuzuAstSyntaxKindInc)
target_link_libraries(YuzuAst PUBLIC YuzuSyntax)
