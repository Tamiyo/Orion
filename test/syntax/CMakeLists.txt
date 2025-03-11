# Create an executable to test this test suite.
add_executable(
        lexer_tests
        lexer/lexer_tests.cc
)

add_executable(
        rgtree_tests
        parser/rgtree/green/green_builder_tests.cc
        parser/rgtree/green/green_cache_tests.cc 
)

# Link GTest to this test suite.
target_link_libraries(
        lexer_tests
        PRIVATE GTest::gtest_main
        PRIVATE syntax
)

target_link_libraries(
        rgtree_tests
        PRIVATE GTest::gtest_main
        PRIVATE syntax
)

gtest_discover_tests(rgtree_tests)
gtest_discover_tests(lexer_tests)