cmake_minimum_required(VERSION 3.16.3)

# Project metadata.
set(CMAKE_C_COMPILER /usr/bin/clang-18)
set(CMAKE_CXX_COMPILER /usr/bin/clang++-18)
project(orion VERSION 0.1 LANGUAGES CXX)
enable_testing()

# Specify the C++ standard.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

if (MSVC)
    # Warning level 4.
    add_compile_options(/W4)
else()
    # Additional warnings.
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Includes.
include(FetchContent)

# Fetch Content.
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Specify source libraries to build.
add_subdirectory(src)
add_subdirectory(test)