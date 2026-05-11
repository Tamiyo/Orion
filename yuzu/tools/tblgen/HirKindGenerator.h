#ifndef YUZU_TOOLS_TBLGEN_HIR_KIND_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_HIR_KIND_GENERATOR_H

#include "CodeGenerator.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>

namespace yuzu::tools {

/// Emits a `HirKind` enum used by LLVM-style RTTI (`classof`,
/// `llvm::isa`, `llvm::dyn_cast`) on the HIR. Each `Variant` section
/// gets `<GROUP>_FIRST`/`<GROUP>_LAST` sentinels around its concrete
/// `Node`s, plus a final `// Nodes` section for `Node`s parented at the
/// `Base` and a `// System` section for `Error`/`Tombstone`.
///
/// Cousin of `SyntaxKindGenerator` but with no tokens block — HIR has
/// no concept of lex tokens — and a separate enum to keep HIR kinds
/// independent from `SyntaxKind` numeric values.
class HirKindGenerator final : public CodeGenerator {
public:
  explicit HirKindGenerator(llvm::raw_ostream &os) : CodeGenerator(os) {}

protected:
  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_HIR_KIND_GENERATOR_H
