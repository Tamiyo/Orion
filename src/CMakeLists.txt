# Build executable.
add_executable(
        orion
        orion/main.cc
)

# Specify source libraries to build.
add_subdirectory(syntax)

# Link libraries.
target_link_libraries(
        orion
        PUBLIC syntax
)

# Link header files.
target_include_directories(
        orion
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/syntax"
)