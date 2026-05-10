add_library(YuzuParser
  Parser.cc
  Grammar/Expr.cc
  Grammar/Grammar.cc
  Grammar/Stmt.cc
)
target_link_libraries(YuzuParser PUBLIC YuzuAst YuzuLexer YuzuSyntax)
