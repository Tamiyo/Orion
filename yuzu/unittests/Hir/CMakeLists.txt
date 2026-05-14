add_yuzu_unittest(HirTests
  HirPrinterTest.cc
  HirLowererTest.cc
)
target_link_libraries(HirTests PRIVATE YuzuHir)
