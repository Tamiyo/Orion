#ifndef YUZU_TOOLS_TBLGEN_HIR_PRINTER_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_HIR_PRINTER_GENERATOR_H

#include "CodeGenerator.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>

namespace yuzu::tools {

/// Emits inline `print<X>(os, node, indent)` free functions for every
/// concrete HIR Node plus a `printNode` dispatcher that switches on
/// `HirKind`. Each `print<X>` writes the node's name, then walks the
/// schema-declared fields one per line: scalars as `field=value`, child
/// nodes by recursing through `printNode`, child lists by iterating.
/// No trailing newline. Hand-written `HirPrinter` reduces to a class
/// wrapping `os` and forwarding to `printNode`.
class HirPrinterGenerator final : public CodeGenerator {
public:
  explicit HirPrinterGenerator(llvm::raw_ostream &os) : CodeGenerator(os) {}

protected:
  void generate(const llvm::RecordKeeper &records) override;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_HIR_PRINTER_GENERATOR_H
