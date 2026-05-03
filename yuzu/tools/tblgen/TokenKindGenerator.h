#ifndef YUZU_TOOLS_TBLGEN_TOKEN_KIND_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_TOKEN_KIND_GENERATOR_H

#include "CodeFormatter.h"
#include "CodeGenerator.h"

#include <llvm/TableGen/Record.h>

#include <utility>

namespace yuzu::tools {

/// Emits the `TokenKind` enum, per-category `is<Category>(TokenKind)`
/// predicates, and `asString(TokenKind)`, driven by every `Token`/`Regex`
/// def in the record keeper (anything derived from `Metadata`). The
/// emitted declarations are wrapped in the namespace declared by the sole
/// `LexGrammar` def in scope.
class TokenKindGenerator final : public CodeGenerator {
public:
  explicit TokenKindGenerator(CodeFormatter fmt)
      : CodeGenerator(std::move(fmt)) {}

  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_TOKEN_KIND_GENERATOR_H
