# The node/kind/builder/visitor generators are tree-agnostic (driven by the
# schema's `TreeName`); ANF reuses them via the `-gen-anf-*` aliases. The HIR
# printer stays HIR-specific, so ANF has no printer yet.
yuzu_tablegen(YuzuAnfKindInc    "yuzu/Anf/AnfKind.h.inc"    -gen-anf-kind-decls    "yuzu/Anf/AnfKind.td")
yuzu_tablegen(YuzuAnfInc        "yuzu/Anf/Anf.h.inc"        -gen-anf-node-decls    "yuzu/Anf/Anf.td")
yuzu_tablegen(YuzuAnfBuilderInc "yuzu/Anf/AnfBuilder.h.inc" -gen-anf-builder-decls "yuzu/Anf/Anf.td")
yuzu_tablegen(YuzuAnfVisitorInc "yuzu/Anf/AnfVisitor.h.inc" -gen-anf-visitor-decls "yuzu/Anf/Anf.td")

# Header-only for now; the HIR->ANF lowerer and passes will add sources later.
# The INTERFACE target carries the codegen dependency and links HIR, since ANF
# reuses `hir::Op` (the type system is header-only via `yuzu::types`).
add_library(YuzuAnf INTERFACE)
add_dependencies(YuzuAnf
  YuzuAnfKindInc YuzuAnfInc YuzuAnfBuilderInc YuzuAnfVisitorInc)
target_link_libraries(YuzuAnf INTERFACE YuzuHir)
