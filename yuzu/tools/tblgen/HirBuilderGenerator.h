#ifndef YUZU_TOOLS_TBLGEN_HIR_BUILDER_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_HIR_BUILDER_GENERATOR_H

#include "CodeGenerator.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>

namespace yuzu::tools {

/// Emits `HirBuilder`: an arena-backed factory with one
/// `makeXxx(fields...)` method per concrete HIR `Node`.
class HirBuilderGenerator final : public CodeGenerator {
public:
  explicit HirBuilderGenerator(llvm::raw_ostream &os) : CodeGenerator(os) {}

protected:
  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_HIR_BUILDER_GENERATOR_H
