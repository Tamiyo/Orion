#ifndef YUZU_TOOLS_TBLGEN_CODE_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_CODE_GENERATOR_H

#include "CodeFormatter.h"

#include <llvm/TableGen/Record.h>

#include <utility>

namespace yuzu::tools {

/// Base for every yuzu-tblgen backend. Owns a `CodeFormatter` (its sole
/// output channel) and exposes `generate(records)` as the entry point each
/// concrete generator implements.
class CodeGenerator {
public:
  explicit CodeGenerator(CodeFormatter fmt) : fmt(std::move(fmt)) {}
  virtual ~CodeGenerator() = default;

  virtual void generate(const llvm::RecordKeeper &records) = 0;

protected:
  CodeFormatter fmt;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_CODE_GENERATOR_H
