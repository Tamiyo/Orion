cmake_minimum_required(VERSION 3.20)
project(yuzu C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_compile_options(-Wall -Wextra -Werror -fno-exceptions -fno-rtti)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# MLIR ships under the same install prefix as LLVM; add it to the search
# path so find_package(MLIR) picks up `<prefix>/lib/cmake/mlir/`.
list(APPEND CMAKE_PREFIX_PATH "${LLVM_INSTALL_PREFIX}")
find_package(MLIR REQUIRED CONFIG)
message(STATUS "Found MLIR ${MLIR_PACKAGE_VERSION}")
message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")

include(YuzuTableGen)

include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${MLIR_INCLUDE_DIRS})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

include_directories(
  ${CMAKE_SOURCE_DIR}/yuzu/include
  ${CMAKE_BINARY_DIR}/include
)

enable_testing()
find_package(GTest REQUIRED)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_subdirectory(yuzu/tools/tblgen)
add_subdirectory(yuzu/lib)
add_subdirectory(yuzu/tools/repl)
add_subdirectory(yuzu/tools/compile)
add_subdirectory(yuzu/unittests)
add_subdirectory(yuzu/test)
