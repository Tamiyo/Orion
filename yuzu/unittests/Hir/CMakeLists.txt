add_yuzu_unittest(HirTests
  LowererTest.cc
)
target_link_libraries(HirTests PRIVATE YuzuHir)
