add_subdirectory(Ast)
add_subdirectory(Compiler)
add_subdirectory(Diagnostics)
add_subdirectory(Hir)
# After Hir: YuzuAnf links YuzuHir (ANF reuses hir::Op).
add_subdirectory(Anf)
# After Anf: YuzuSubstrait consumes reduced ANF.
add_subdirectory(Substrait)
add_subdirectory(Lexer)
add_subdirectory(Parser)
add_subdirectory(Syntax)
add_subdirectory(Util)