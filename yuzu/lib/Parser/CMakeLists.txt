add_library(YuzuParser
  Parser.cc
  Grammar/Expr.cc
)
target_link_libraries(YuzuParser PUBLIC YuzuAst YuzuLexer YuzuSyntax)
