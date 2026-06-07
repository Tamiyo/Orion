# Lit-based golden tests, run through FileCheck.
#
#   tools/tblgen/<Generator>/*.td   yuzu-tblgen output
#   yuzu/*.yz                        Yuzu programs (yuzu-compile output)
#
# Each test file carries its own `RUN:` lines and `CHECK:` directives.

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
  NAME yuzu-lit
  COMMAND ${Python3_EXECUTABLE} ${LIT_PY} -sv
          "${CMAKE_CURRENT_BINARY_DIR}"
)

add_custom_target(check-yuzu-lit
  COMMAND ${Python3_EXECUTABLE} ${LIT_PY} -sv
          "${CMAKE_CURRENT_BINARY_DIR}"
  DEPENDS yuzu-tblgen yuzu-compile
  USES_TERMINAL
  COMMENT "Running yuzu lit tests"
)
