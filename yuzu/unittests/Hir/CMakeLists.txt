add_yuzu_unittest(HirTests
  BuiltinOpsTest.cc
  HirContextTest.cc
  TypeInternerTest.cc
  TypeCoercionTest.cc
)
target_link_libraries(HirTests PRIVATE YuzuHir YuzuDiagnostics)
