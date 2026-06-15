#ifndef YUZU_TOOLS_TBLGEN_ANF_PRINTER_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_ANF_PRINTER_GENERATOR_H

#include "CodeGenerator.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>

namespace yuzu::tools {

/// Emits inline `print<X>(os, node, indent)` free functions for every concrete
/// ANF Node plus a `printNode` dispatcher that switches on `AnfKind`. The
/// counterpart of `HirPrinterGenerator`, but context-free: ANF carries its
/// types on each node, so a node's type is read from its own `type` field and
/// printed inline as ` : <type>` rather than looked up in a side table.
class AnfPrinterGenerator final : public CodeGenerator {
public:
  explicit AnfPrinterGenerator(llvm::raw_ostream &os) : CodeGenerator(os) {}

protected:
  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_ANF_PRINTER_GENERATOR_H
