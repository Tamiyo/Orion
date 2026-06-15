cmake_minimum_required(VERSION 3.20)
project(yuzu C CXX)


if (CMAKE_VERSION VERSION_LESS 3.18)
  set(DEV_MODULE Development)
else()
  set(DEV_MODULE Development.Module)
endif()

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# Configures CMake to perform an optimized release build by default 
# unless another build type is specified. Without this addition, 
# binding code may run slowly and produce large binaries.
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(RegisterLLVM)
include(RegisterMLIR)
include(RegisterNanobind)

# Third party libaries.
find_package(Python 3.12 COMPONENTS Interpreter ${DEV_MODULE} REQUIRED)

register_llvm()
register_mlir()
register_nanobind()

# C/C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# The core static libs are linked into the nanobind Python module (a shared
# object), so everything must be position-independent.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
add_compile_options(-Wall -Wextra -Werror -fno-exceptions -fno-rtti)

# Yuzu-related dependencies.
include(YuzuTableGen)

include_directories(
  ${CMAKE_SOURCE_DIR}/yuzu/include
  ${CMAKE_BINARY_DIR}/include
)

enable_testing()
find_package(GTest REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_subdirectory(yuzu)