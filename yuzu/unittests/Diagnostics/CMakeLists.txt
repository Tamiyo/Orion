add_yuzu_unittest(DiagnosticsTests
  DiagnosticPrinterTest.cc
  DiagnosticsEngineTest.cc
  SourceMapTest.cc
)
target_link_libraries(DiagnosticsTests PRIVATE YuzuDiagnostics)
