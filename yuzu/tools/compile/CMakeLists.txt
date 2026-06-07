add_executable(yuzu-compile
  Compile.cc
)
target_link_libraries(yuzu-compile PRIVATE Yuzu YuzuUtil)