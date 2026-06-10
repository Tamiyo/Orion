add_yuzu_unittest(AnfTests
  AnfBuilderTest.cc
)
target_link_libraries(AnfTests PRIVATE YuzuAnf YuzuHir)
# Generated ANF headers must exist before the test compiles.
add_dependencies(AnfTests
  YuzuAnfKindInc YuzuAnfInc YuzuAnfBuilderInc YuzuAnfVisitorInc)
