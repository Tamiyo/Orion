# Configured library.
add_library(
        syntax
        lexer/abstract_lexer.cc
        lexer/lexer.cc
        parser/rgtree/green/green_builder.cc
        parser/rgtree/green/green_cache.cc
        parser/rgtree/green/green_node.cc
)

# Link header files.
target_include_directories(
        syntax
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/.."
)