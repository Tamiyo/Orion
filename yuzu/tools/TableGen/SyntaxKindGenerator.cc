#include "yuzu/tools/TableGen/SyntaxKindGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <algorithm>
#include <vector>

namespace yuzu_tools {
void SyntaxKindGenerator::emitClassDefinitions(
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

  os << "enum class SyntaxKind : uint16_t {\n";
  os << "  // Tokens\n";
  for (const auto &token : tokens) {
    const llvm::StringRef name = token->getValueAsString("Name");
    os << "  " << name << ",\n";
  }
  os << "\n";

  // Generate node kinds.
  std::vector<const llvm::Record *> nodes =
      grammar->getValueAsListOfDefs("Nodes");

  std::sort(nodes.begin(), nodes.end(),
            [](const llvm::Record *a, const llvm::Record *b) {
              const llvm::StringRef aName = a->getValueAsString("Name");
              const llvm::StringRef bName = b->getValueAsString("Name");
              return std::lexicographical_compare(aName.begin(), aName.end(),
                                                  bName.begin(), bName.end());
            });

  assert(nodes.size() < sizeof(uint16_t));

  os << "  // Nodes\n";
  for (const auto &node : nodes) {
    const llvm::StringRef name = node->getValueAsString("Name");
    os << "  " << name << ",\n";
  }
  os << "\n";

  // Genereate system kinds
  os << "  // System\n";
  os << "  Error";

  os << "};\n";
}

void SyntaxKindGenerator::emitHeader(llvm::raw_ostream &os) const noexcept {
  emitSourceFileHeader("Yuzu SyntaxKind Declarations", os);

  emitOpenIncludeGuards(os);
  emitIncludes(os);

  os << "namespace yuzu::ast {\n";

  emitClassDefinitions(os);

  os << "} // namespace yuzu::ast\n\n";

  emitCloseIncludeGuards(os);
}

void SyntaxKindGenerator::runImpl(llvm::raw_ostream &os) const noexcept {
  emitHeader(os);
}

} // namespace yuzu_tools
