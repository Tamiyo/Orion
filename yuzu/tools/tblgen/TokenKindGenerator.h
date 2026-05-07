#ifndef YUZU_TOOLS_TBLGEN_TOKEN_KIND_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_TOKEN_KIND_GENERATOR_H

#include "CodeGenerator.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>

namespace yuzu::tools {

/// Emits the `TokenKind` enum, per-category `is<Category>(TokenKind)`
/// predicates, and `asString(TokenKind)`, driven by every `Token`/`Regex`
/// def in the record keeper (anything derived from `Metadata`). The
/// emitted declarations are wrapped in the namespace declared by the sole
/// `LexGrammar` def in scope.
class TokenKindGenerator final : public CodeGenerator {
public:
  explicit TokenKindGenerator(llvm::raw_ostream &os) : CodeGenerator(os) {}

protected:
  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_TOKEN_KIND_GENERATOR_H
