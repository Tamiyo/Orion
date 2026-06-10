macro(register_mlir)
    # MLIR ships under the same install prefix as LLVM; add it to the search
    # path so find_package(MLIR) picks up `<prefix>/lib/cmake/mlir/`.
    list(APPEND CMAKE_PREFIX_PATH "${LLVM_INSTALL_PREFIX}")
    find_package(MLIR REQUIRED CONFIG)
    message(STATUS "Found MLIR ${MLIR_PACKAGE_VERSION}")
    message(STATUS "Using MLIRConfig.cmake in: ${MLIR_DIR}")
endmacro()