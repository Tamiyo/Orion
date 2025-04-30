# Create an executable to test this test suite.
add_executable(
        syntax_parser_tests
        syntax_kind_tests.cc
        parser/token_sink_tests.cc
        parser/token_source_tests.cc
)

add_executable(
        syntax_rgtree_tests
        rgtree/green_builder_tests.cc
        rgtree/green_cache_tests.cc 
        rgtree/green_node_tests.cc
        rgtree/green_token_tests.cc
        rgtree/green_writer_tests.cc 
)

# Link GTest to this test suite.
target_link_libraries(
        syntax_parser_tests
        PRIVATE GTest::gtest_main
        PRIVATE syntax
)

target_link_libraries(
        syntax_rgtree_tests
        PRIVATE GTest::gtest_main
        PRIVATE syntax
)

# Discover Tests
gtest_discover_tests(syntax_parser_tests)
gtest_discover_tests(syntax_rgtree_tests)
