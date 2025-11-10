#include "yuzu/tools/TableGen/TokenKindGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <algorithm>
#include <vector>

namespace yuzu_tools {
void TokenKindGenerator::emitClassDefinitions(
    llvm::raw_ostream &os) const noexcept {
  const llvm::Record *grammar = records.getDef("YuzuGrammar");

  // Generate token kinds.
  std::vector<const llvm::Record *> tokens =
      grammar->getValueAsListOfDefs("Tokens");

  std::sort(tokens.begin(), tokens.end(),
            [](const llvm::Record *a, const llvm::Record *b) {
              const llvm::StringRef aName = a->getValueAsString("Name");
              const llvm::StringRef bName = b->getValueAsString("Name");
              return std::lexicographical_compare(aName.begin(), aName.end(),
                                                  bName.begin(), bName.end());
            });

  assert(tokens.size() < sizeof(uint16_t));

  os << "enum class TokenKind : uint16_t {\n";
  os << "  // Tokens\n";
  for (const auto &token : tokens) {
    const llvm::StringRef name = token->getValueAsString("Name");
    os << "  " << name << ",\n";
  }
  os << "\n";

  // Genereate system kinds
  os << "  // System\n";
  os << "  Error";

  os << "};\n";
}

void TokenKindGenerator::emitHeader(llvm::raw_ostream &os) const noexcept {
  emitSourceFileHeader("Yuzu TokenKind Declarations", os);

  emitOpenIncludeGuards(os);
  emitIncludes(os);

  os << "namespace yuzu::lexer {\n";

  emitClassDefinitions(os);

  os << "} // namespace yuzu::lexer\n\n";

  emitCloseIncludeGuards(os);
}

void TokenKindGenerator::runImpl(llvm::raw_ostream &os) const noexcept {
  emitHeader(os);
}

} // namespace yuzu_tools
