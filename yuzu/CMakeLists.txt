# tblgen must come first: lib (and everything downstream) consumes the
# HIR headers it generates.
add_subdirectory(tools/tblgen)
add_subdirectory(lib)
add_subdirectory(tools/repl)
add_subdirectory(tools/compile)
add_subdirectory(unittests)
add_subdirectory(test)

# Python bindings — built with RTTI/exceptions, scoped entirely to its own dir.
add_subdirectory(python)