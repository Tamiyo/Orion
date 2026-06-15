add_yuzu_unittest(AnfTests
  AnfLowererTest.cc
  AnfReducerTest.cc
)
target_link_libraries(AnfTests PRIVATE YuzuAnf YuzuHir YuzuDiagnostics)
# Generated ANF headers must exist before the test compiles.
add_dependencies(AnfTests
  YuzuAnfKindInc YuzuAnfInc YuzuAnfBuilderInc YuzuAnfVisitorInc YuzuAnfPrinterInc)
