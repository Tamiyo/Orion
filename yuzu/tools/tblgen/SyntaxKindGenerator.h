#ifndef YUZU_TOOLS_TBLGEN_SYNTAX_KIND_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_SYNTAX_KIND_GENERATOR_H

#include "CodeFormatter.h"
#include "CodeGenerator.h"

#include <llvm/TableGen/Record.h>

#include <utility>

namespace yuzu::tools {

/// Emits a `SyntaxKind` enum that unions every lex token (`Metadata`-derived
/// def) with every AST kind (each `Variant` and the concrete `Node`s parented
/// under it). Each section ends with `<GROUP>_FIRST`/`<GROUP>_LAST` sentinels
/// so callers can do range checks like
/// `kind > EXPR_FIRST && kind < EXPR_LAST`. The emitted enum is wrapped in
/// the namespace declared on the AST root `Base` def in scope.
class SyntaxKindGenerator final : public CodeGenerator {
public:
  explicit SyntaxKindGenerator(CodeFormatter fmt)
      : CodeGenerator(std::move(fmt)) {}

  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_SYNTAX_KIND_GENERATOR_H
