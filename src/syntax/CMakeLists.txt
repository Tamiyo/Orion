# Configured library.
add_library(
        syntax INTERFACE
)

# Link header files.
target_include_directories(
        syntax 
        INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
