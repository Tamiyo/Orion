#include "SyntaxKindGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>

namespace yuzu_tools {
void SyntaxKindGenerator::emitClassDefinitions(llvm::raw_ostream &os) const {
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

  assert(tokens.size() <= std::numeric_limits<uint16_t>::max() &&
         "Token count exceeds the range of SyntaxKind's uint16_t backing.");

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

  assert(nodes.size() <= std::numeric_limits<uint16_t>::max() &&
         "Node count exceeds the range of SyntaxKind's uint16_t backing.");

  os << "  // Nodes\n";
  for (const auto &node : nodes) {
    const llvm::StringRef name = node->getValueAsString("Name");
    os << "  " << name << ",\n";
  }
  os << "\n";

  // Genereate system kinds
  os << "  // System\n";
  os << "  Error,";
  os << "  Tombstone";

  os << "};\n";
}

void SyntaxKindGenerator::emitHeader(llvm::raw_ostream &os) const {
  emitSourceFileHeader("Yuzu SyntaxKind Declarations", os);

  emitOpenIncludeGuards(os);
  emitIncludes(os);

  os << "namespace yuzu::ast {\n";

  emitClassDefinitions(os);

  os << "} // namespace yuzu::ast\n\n";

  emitCloseIncludeGuards(os);
}

void SyntaxKindGenerator::runImpl(llvm::raw_ostream &os) const {
  emitHeader(os);
}

} // namespace yuzu_tools
