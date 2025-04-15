# Configured library.
add_library(
        lang
        lexer/lexer.cc
        parser/grammar/expression/expression.cc
        parser/grammar/statement/statement.cc
)

target_link_libraries(
        lang
        INTERFACE syntax
)

# Link header files.
target_include_directories(
        lang
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
