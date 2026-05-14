yuzu_tablegen(YuzuHirKindInc    "yuzu/Hir/HirKind.h.inc"    -gen-hir-kind-decls    "yuzu/Hir/HirKind.td")
yuzu_tablegen(YuzuHirInc        "yuzu/Hir/Hir.h.inc"        -gen-hir-node-decls    "yuzu/Hir/Hir.td")
yuzu_tablegen(YuzuHirBuilderInc "yuzu/Hir/HirBuilder.h.inc" -gen-hir-builder-decls "yuzu/Hir/Hir.td")

add_library(YuzuHir
  HirPrinter.cc
  HirLowerer.cc
)
add_dependencies(YuzuHir YuzuHirKindInc YuzuHirInc YuzuHirBuilderInc)
target_link_libraries(YuzuHir PUBLIC YuzuAst YuzuDiagnostics YuzuUtil)
