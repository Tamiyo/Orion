add_library(YuzuSyntax
  Syntax.cc
  Green/Green.cc
  Green/GreenBuilder.cc
  Green/GreenCache.cc
)
target_link_libraries(YuzuSyntax PUBLIC YuzuUtil)
