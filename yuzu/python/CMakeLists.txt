# Python bindings (nanobind). This is the ONLY part of the tree built with RTTI
# and exceptions: nanobind's templates use typeid and throw, which the core
# build forbids via the global -fno-rtti / -fno-exceptions.
#
# Those global flags are inherited here as a directory property, so we re-enable
# RTTI/exceptions for everything in this subdirectory. The override is scoped to
# this directory's targets (the module plus nanobind's internal runtime lib) and
# never reaches the LLVM-style core. Later flags win on the command line.
add_compile_options(-frtti -fexceptions)

# The native module is `_ext`; the importable package is the `yuzu/`
# directory beside this file. Emit the built `.so` *into* that package dir
# so `import yuzu` works straight from the source tree (no install step) —
# the built artifact is git-ignored.
nanobind_add_module(_ext NOMINSIZE YuzuExt.cc)
set_target_properties(_ext PROPERTIES
  LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/yuzu)

# Type stub for the native module, so Pylance/pyright can resolve
# `from ._ext import *` and offer completion. Regenerated whenever `_ext`
# is rebuilt.
nanobind_add_stub(_ext_stub
  MODULE _ext
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/yuzu/_ext.pyi
  PYTHON_PATH ${CMAKE_CURRENT_SOURCE_DIR}/yuzu
  DEPENDS _ext)
