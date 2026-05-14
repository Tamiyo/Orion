add_library(Yuzu
  CompilePipeline.cc
)
target_link_libraries(Yuzu PUBLIC
  YuzuAst
  YuzuCodegen
  YuzuDiagnostics
  YuzuHir
  YuzuLexer
  YuzuParser
  YuzuUtil
)
