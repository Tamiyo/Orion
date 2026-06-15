add_library(Yuzu
  CompilePipeline.cc
)
target_link_libraries(Yuzu PUBLIC
  YuzuAnf
  YuzuAst
  YuzuDiagnostics
  YuzuHir
  YuzuLexer
  YuzuParser
  YuzuSubstrait
  YuzuUtil
)
