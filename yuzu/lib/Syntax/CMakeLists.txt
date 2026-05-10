add_library(YuzuSyntax
  Detail/Syntax.cc
  Green/Green.cc
  Green/GreenBuilder.cc
  Green/GreenCache.cc
  Green/GreenPrinter.cc
)
target_link_libraries(YuzuSyntax PUBLIC YuzuUtil)
