#include "yuzu/tools/TableGen/TokenKindGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <algorithm>
#include <string>
#include <vector>

namespace yuzu_tools {
void TokenKindGenerator::emitClassDefinitions(
    llvm::raw_ostream &os) const noexcept {
  const llvm::Record *grammar = records.getDef("YuzuGrammar");

  auto sortAndEmitRecords = [&os](const std::string &subclass,
                                  const std::string &tag,
                                  std::vector<const llvm::Record *> &tokens) {
    std::vector<const llvm::Record *> records;
    std::copy_if(tokens.begin(), tokens.end(), std::back_inserter(records),
                 [&subclass](const llvm::Record *record) {
                   return record->isSubClassOf(subclass);
                 });

    std::sort(records.begin(), records.end(),
              [](const llvm::Record *a, const llvm::Record *b) {
                const llvm::StringRef aName = a->getValueAsString("Name");
                const llvm::StringRef bName = b->getValueAsString("Name");
                return std::lexicographical_compare(aName.begin(), aName.end(),
                                                    bName.begin(), bName.end());
              });

    os << "  // " << tag << "\n";
    for (const auto &token : records) {
      const llvm::StringRef name = token->getValueAsString("Name");
      os << "  " << name << ",\n";
    }
    os << "\n";
  };

  // Generate token kinds.
  std::vector<const llvm::Record *> tokens =
      grammar->getValueAsListOfDefs("Tokens");

  assert(tokens.size() < sizeof(uint16_t));

  os << "enum class TokenKind : uint16_t {\n";
  sortAndEmitRecords("SymbolToken", "Symbols", tokens);
  sortAndEmitRecords("LiteralToken", "Literals", tokens);
  sortAndEmitRecords("TriviaToken", "Trivia", tokens);
  sortAndEmitRecords("KeywordToken", "Keywords", tokens);
  sortAndEmitRecords("BasicToken", "Other", tokens);

  // Genereate system kinds
  os << "  // System\n";
  os << "  Error";

  os << "};\n";
}

void TokenKindGenerator::emitInlineMethods(
    llvm::raw_ostream &os) const noexcept {
  const llvm::Record *grammar = records.getDef("YuzuGrammar");

  auto sortAndEmitInlineMethods =
      [&os](const std::string &subclass, const std::string &methodName,
            std::vector<const llvm::Record *> &tokens) {
        std::vector<const llvm::Record *> records;
        std::copy_if(tokens.begin(), tokens.end(), std::back_inserter(records),
                     [&subclass](const llvm::Record *record) {
                       return record->isSubClassOf(subclass);
                     });

        std::sort(records.begin(), records.end(),
                  [](const llvm::Record *a, const llvm::Record *b) {
                    const llvm::StringRef aName = a->getValueAsString("Name");
                    const llvm::StringRef bName = b->getValueAsString("Name");
                    return std::lexicographical_compare(
                        aName.begin(), aName.end(), bName.begin(), bName.end());
                  });

        os << "\ninline bool is" << methodName << "(TokenKind kind) {"
           << "\n";
        os << "  switch(kind) {";
        for (const auto &token : records) {
          const llvm::StringRef name = token->getValueAsString("Name");
          os << "    case TokenKind::" << name << ":\n";
        }
        os << "      return true;\n";
        os << "    default:\n";
        os << "      return false;\n";
        os << "  }\n";
        os << "}\n";
      };

  // Generate token kinds.
  std::vector<const llvm::Record *> tokens =
      grammar->getValueAsListOfDefs("Tokens");

  assert(tokens.size() < sizeof(uint16_t));

  sortAndEmitInlineMethods("SymbolToken", "Symbol", tokens);
  sortAndEmitInlineMethods("LiteralToken", "Literal", tokens);
  sortAndEmitInlineMethods("TriviaToken", "Trivia", tokens);
  sortAndEmitInlineMethods("KeywordToken", "Keyword", tokens);
}

void TokenKindGenerator::emitHeader(llvm::raw_ostream &os) const noexcept {
  emitSourceFileHeader("Yuzu TokenKind Declarations", os);

  emitOpenIncludeGuards(os);
  emitIncludes(os);

  os << "namespace yuzu::lexer {\n";

  emitClassDefinitions(os);
  os << "\n";
  emitInlineMethods(os);

  os << "} // namespace yuzu::lexer\n\n";

  emitCloseIncludeGuards(os);
}

void TokenKindGenerator::runImpl(llvm::raw_ostream &os) const noexcept {
  emitHeader(os);
}

} // namespace yuzu_tools
