# Create an executable to test this test suite.
add_executable(
        lang_lexer_tests
        lexer/lexer_tests.cc
)

add_executable(
        lang_parser_tests
        parser/grammar/expression/expression_tests.cc
)

# Link GTest to this test suite.
target_link_libraries(
        lang_lexer_tests
        PRIVATE GTest::gtest_main
        PRIVATE lang
)

target_link_libraries(
        lang_parser_tests
        PRIVATE GTest::gtest_main
        PRIVATE lang
)

# Discover Tests
gtest_discover_tests(lang_lexer_tests)
gtest_discover_tests(lang_parser_tests)
