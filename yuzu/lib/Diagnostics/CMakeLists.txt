add_library(YuzuDiagnostics
  DiagnosticBuilder.cc
  DiagnosticPrinter.cc
  SourceMap.cc
)
target_link_libraries(YuzuDiagnostics PUBLIC YuzuUtil)
