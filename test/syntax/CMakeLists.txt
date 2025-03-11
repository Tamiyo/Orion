# Create an executable to test this test suite.
add_executable(
        syntax_tests
        syntax_tests.cc
        lexer/lexer_tests.cc
)

# Link GTest to this test suite.
target_link_libraries(
        syntax_tests
        PRIVATE GTest::gtest_main
        PRIVATE syntax
)

gtest_discover_tests(syntax_tests)