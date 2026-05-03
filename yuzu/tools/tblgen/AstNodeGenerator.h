#ifndef YUZU_TOOLS_TBLGEN_AST_NODE_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_AST_NODE_GENERATOR_H

#include "CodeFormatter.h"
#include "CodeGenerator.h"

#include <llvm/TableGen/Record.h>

#include <utility>

namespace yuzu::tools {

/// Emits AST node and variant classes from the `Variant`/`Node` defs in the
/// record keeper. Concrete `Node` defs become final `AstNode<T>` subclasses
/// with one accessor per field; `Variant` defs become `std::variant`
/// aggregates over their concrete children.
class AstNodeGenerator final : public CodeGenerator {
public:
  explicit AstNodeGenerator(CodeFormatter fmt)
      : CodeGenerator(std::move(fmt)) {}

  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_AST_NODE_GENERATOR_H
