yuzu_tablegen(YuzuHirKindInc    "yuzu/Hir/HirKind.h.inc"    -gen-hir-kind-decls    "yuzu/Hir/HirKind.td")
yuzu_tablegen(YuzuHirInc        "yuzu/Hir/Hir.h.inc"        -gen-hir-node-decls    "yuzu/Hir/Hir.td")
yuzu_tablegen(YuzuHirBuilderInc "yuzu/Hir/HirBuilder.h.inc" -gen-hir-builder-decls "yuzu/Hir/Hir.td")
yuzu_tablegen(YuzuHirPrinterInc "yuzu/Hir/HirPrinter.h.inc" -gen-hir-printer-decls "yuzu/Hir/Hir.td")
yuzu_tablegen(YuzuHirVisitorInc "yuzu/Hir/HirVisitor.h.inc" -gen-hir-visitor-decls "yuzu/Hir/Hir.td")

add_library(YuzuHir
  HirLowerer.cc
  Ops/BuiltinOps.cc
  Resolve/HirScope.cc
  Resolve/HirSymbolTable.cc
  Types/TypeResolver.cc
  Types/TypeInferrer.cc
  Types/TypeCoercion.cc
)
add_dependencies(YuzuHir YuzuHirKindInc YuzuHirInc YuzuHirBuilderInc YuzuHirPrinterInc YuzuHirVisitorInc)
target_link_libraries(YuzuHir PUBLIC YuzuAst YuzuDiagnostics YuzuUtil)
