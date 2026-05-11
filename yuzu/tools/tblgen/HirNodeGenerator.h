#ifndef YUZU_TOOLS_TBLGEN_HIR_NODE_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_HIR_NODE_GENERATOR_H

#include "CodeGenerator.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>

namespace yuzu::tools {

/// Emits the HIR C++ class hierarchy: a `HirKind`-carrying base, `Variant`
/// classes with `isA` / `cast`, and concrete `Node` classes that own their
/// fields by value.
class HirNodeGenerator final : public CodeGenerator {
public:
  explicit HirNodeGenerator(llvm::raw_ostream &os) : CodeGenerator(os) {}

protected:
  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_HIR_NODE_GENERATOR_H
