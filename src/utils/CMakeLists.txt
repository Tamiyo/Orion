# Configured library.
add_library(
        utils INTERFACE
)

# Link header files.
target_include_directories(
        utils 
        INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
