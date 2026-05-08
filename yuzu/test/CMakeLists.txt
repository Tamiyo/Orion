# Lit-based golden tests for yuzu-tblgen.
#
# Tests live under tools/tblgen/<Generator>/ — each `.td` file contains its
# own `RUN:` lines and `CHECK:` directives.

find_program(FILECHECK_BIN
  NAMES FileCheck FileCheck-20
  HINTS ${LLVM_TOOLS_BINARY_DIR} /usr/lib/llvm-20/bin
  REQUIRED
)

find_program(LIT_PY
  NAMES lit lit.py
  HINTS ${LLVM_TOOLS_BINARY_DIR} /usr/lib/llvm-20/build/utils/lit
  REQUIRED
)

configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/lit.site.cfg.py.in"
  "${CMAKE_CURRENT_BINARY_DIR}/lit.site.cfg.py"
  @ONLY
)

add_test(
  NAME yuzu-tblgen-lit
  COMMAND ${Python3_EXECUTABLE} ${LIT_PY} -sv
          "${CMAKE_CURRENT_BINARY_DIR}"
)

add_custom_target(check-yuzu-tblgen
  COMMAND ${Python3_EXECUTABLE} ${LIT_PY} -sv
          "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS yuzu-tblgen
  USES_TERMINAL
  COMMENT "Running yuzu-tblgen lit tests"
)
