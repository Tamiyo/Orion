# Configured library.
add_library(
        ast INTERFACE
)

# Link header files.
target_include_directories(
        ast 
        INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
