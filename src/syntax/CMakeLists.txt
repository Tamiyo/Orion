# Configured library.
add_library(
        syntax
        lexer/abstract_lexer.cc
        lexer/lexer.cc
)

# Link header files.
target_include_directories(
        syntax
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/.."
)