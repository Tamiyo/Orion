add_executable(yuzu-repl
  Repl.cc
)
target_link_libraries(yuzu-repl PRIVATE Yuzu YuzuUtil)

# `cmake --build . --target repl` (or `ninja repl`) builds yuzu-repl and
# launches it interactively. `USES_TERMINAL` hands the REPL the real
# tty for stdin/stdout; without it Ninja would pipe both and getline
# would never see keystrokes.
add_custom_target(repl
  COMMAND $<TARGET_FILE:yuzu-repl>
  DEPENDS yuzu-repl
  USES_TERMINAL
  COMMENT "Running yuzu-repl"
)
