# Build executable.
add_executable(
        yuzu
        yuzu/main.cc
)

# Specify source libraries to build.
add_subdirectory(lang)
add_subdirectory(syntax)

# Link libraries.
target_link_libraries(
        yuzu
        PUBLIC lang
        PUBLIC syntax
)

# Link header files.
target_include_directories(
        yuzu
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/lang"
        PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/syntax"
)
