# Configured library.
add_library(
        util
        utf8_string_utils.cc
)

# Link header files.
target_include_directories(
        util
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/.."
)