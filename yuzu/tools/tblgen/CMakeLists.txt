llvm_map_components_to_libnames(yuzu_tblgen_llvm_libs
  Support
  TableGen
)

add_executable(yuzu-tblgen
  AstNodeGenerator.cc
  HirBuilderGenerator.cc
  HirKindGenerator.cc
  HirNodeGenerator.cc
  HirPrinterGenerator.cc
  HirVisitorGenerator.cc
  SyntaxKindGenerator.cc
  TextMateGrammarGenerator.cc
  TokenKindGenerator.cc
  YuzuGenAst.cc
)

target_include_directories(yuzu-tblgen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(yuzu-tblgen PRIVATE ${yuzu_tblgen_llvm_libs})
