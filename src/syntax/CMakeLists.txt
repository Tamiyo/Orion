# Configured library.
add_library(
        syntax
        lexer.cc
        lexer/lexer_base.cc
        parser/parser_base.cc
        parser/token_sink.cc
        parser/rgtree/green/green.cc
        parser/rgtree/green/green_builder.cc
        parser/rgtree/green/green_cache.cc
        parser/rgtree/green/green_writer.cc
)

# Link header files.
target_include_directories(
        syntax
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
