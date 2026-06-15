add_yuzu_unittest(SubstraitTests
  SubstraitEmitterTest.cc
)
target_link_libraries(SubstraitTests PRIVATE
  YuzuSubstrait YuzuAnf YuzuHir YuzuDiagnostics)
# Generated ANF headers must exist before the test compiles.
add_dependencies(SubstraitTests
  YuzuAnfKindInc YuzuAnfInc YuzuAnfBuilderInc YuzuAnfVisitorInc YuzuAnfPrinterInc)
