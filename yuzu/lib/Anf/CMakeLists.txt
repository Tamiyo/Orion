# The node/kind/builder/visitor generators are tree-agnostic (driven by the
# schema's `TreeName`); ANF reuses them via the `-gen-anf-*` aliases. The
# printer is its own ANF-specific generator (context-free; types are on-node).
yuzu_tablegen(YuzuAnfKindInc    "yuzu/Anf/AnfKind.h.inc"    -gen-anf-kind-decls    "yuzu/Anf/AnfKind.td")
yuzu_tablegen(YuzuAnfInc        "yuzu/Anf/Anf.h.inc"        -gen-anf-node-decls    "yuzu/Anf/Anf.td")
yuzu_tablegen(YuzuAnfBuilderInc "yuzu/Anf/AnfBuilder.h.inc" -gen-anf-builder-decls "yuzu/Anf/Anf.td")
yuzu_tablegen(YuzuAnfVisitorInc "yuzu/Anf/AnfVisitor.h.inc" -gen-anf-visitor-decls "yuzu/Anf/Anf.td")
yuzu_tablegen(YuzuAnfPrinterInc "yuzu/Anf/AnfPrinter.h.inc" -gen-anf-printer-decls "yuzu/Anf/Anf.td")

# ANF shares the `BuiltinOp` enum with HIR (`yuzu/Ops/BuiltinOp.h`) and adds its
# own constant-folding (`Ops/OpFold.cc`); it links HIR because the lowerer reads
# HIR nodes and both share the type system.
add_library(YuzuAnf
  AnfExprLowerer.cc
  AnfLowerer.cc
  AnfReducer.cc
  AnfRelLowerer.cc
  AnfStmtLowerer.cc
  Ops/OpFold.cc
)
add_dependencies(YuzuAnf
  YuzuAnfKindInc YuzuAnfInc YuzuAnfBuilderInc YuzuAnfVisitorInc YuzuAnfPrinterInc)
target_link_libraries(YuzuAnf PUBLIC YuzuHir)
