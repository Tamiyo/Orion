function(add_yuzu_unittest test_name)
  add_executable(${test_name} ${ARGN})
  target_link_libraries(${test_name}
    PRIVATE
      GTest::gtest
      GTest::gtest_main
      YuzuAst
      YuzuLexer
      YuzuParser
      YuzuSyntax
      YuzuUtil
  )
  add_test(NAME ${test_name} COMMAND ${test_name})
endfunction()

add_subdirectory(Ast)
add_subdirectory(Codegen)
add_subdirectory(Diagnostics)
add_subdirectory(Driver)
add_subdirectory(Hir)
add_subdirectory(Lexer)
add_subdirectory(Parser)
add_subdirectory(Syntax)
add_subdirectory(Util)
