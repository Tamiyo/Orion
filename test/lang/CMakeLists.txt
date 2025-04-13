# Create an executable to test this test suite.
add_executable(
        lexer_tests
        lexer_tests.cc
)


# Link GTest to this test suite.
target_link_libraries(
        lexer_tests
        PRIVATE GTest::gtest_main
        PRIVATE lang
)

# Discover Tests
gtest_discover_tests(lexer_tests)
