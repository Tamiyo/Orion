#ifndef YUZU_ANF_ANFPRINTER_H
#define YUZU_ANF_ANFPRINTER_H

#include "yuzu/Anf/Anf.h"
#include "yuzu/Types/Type.h"         // IWYU pragma: keep (types::asString)
#include "yuzu/Util/ErrorHandling.h" // IWYU pragma: keep
#include "yuzu/Util/Unicode.h"       // IWYU pragma: keep (util::writeUtf8)

#include <llvm/Support/raw_ostream.h>

#include <string>

#include "yuzu/Anf/AnfPrinter.h.inc" // IWYU pragma: export

namespace yuzu::anf {

class [[nodiscard]] AnfPrinter final {
public:
  explicit AnfPrinter(llvm::raw_ostream &os) : os(os) {}
  AnfPrinter() = delete;

  void print(const AnfNode *node) { printNode(os, node, 0); }

  static std::string printToString(const AnfNode *node) {
    std::string out;
    llvm::raw_string_ostream stream(out);
    AnfPrinter(stream).print(node);
    return out;
  }

private:
  llvm::raw_ostream &os;
};

} // namespace yuzu::anf

#endif // YUZU_ANF_ANFPRINTER_H
