# Create an executable to test this test suite.
add_executable(
        lexer_tests
        lexer_tests.cc
)

add_executable(
        parser_tests
        syntax_kind_tests.cc
        parser/token_sink_tests.cc
        parser/token_source_tests.cc
)

add_executable(
        rgtree_tests
        parser/rgtree/green/green_builder_tests.cc
        parser/rgtree/green/green_cache_tests.cc 
        parser/rgtree/green/green_node_tests.cc
        parser/rgtree/green/green_token_tests.cc
        parser/rgtree/green/green_writer_tests.cc 
)

# Link GTest to this test suite.
target_link_libraries(
        lexer_tests
        PRIVATE GTest::gtest_main
        PRIVATE syntax
)

target_link_libraries(
        parser_tests
        PRIVATE GTest::gtest_main
        PRIVATE syntax
)

target_link_libraries(
        rgtree_tests
        PRIVATE GTest::gtest_main
        PRIVATE syntax
)

# Discover Tests
gtest_discover_tests(lexer_tests)
gtest_discover_tests(parser_tests)
gtest_discover_tests(rgtree_tests)
