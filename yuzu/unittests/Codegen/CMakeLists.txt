add_yuzu_unittest(CodegenTests
  HirDiagnosticHandlerTest.cc
  HirToMlirTest.cc
)
target_link_libraries(CodegenTests PRIVATE YuzuCodegen YuzuHir)
