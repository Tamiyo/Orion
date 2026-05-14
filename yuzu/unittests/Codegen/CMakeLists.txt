add_yuzu_unittest(CodegenTests
  HirToMlirTest.cc
)
target_link_libraries(CodegenTests PRIVATE YuzuCodegen YuzuHir)
