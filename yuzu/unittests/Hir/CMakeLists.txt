add_yuzu_unittest(HirTests
  BuiltinOpsTest.cc
  HirContextTest.cc
  HirVisitorTest.cc
  TypeCoercionTest.cc
  UnificationTableTest.cc
)
target_link_libraries(HirTests PRIVATE YuzuHir YuzuDiagnostics)
