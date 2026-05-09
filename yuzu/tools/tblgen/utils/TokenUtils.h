#ifndef YUZU_TOOLS_TBLGEN_UTILS_TOKEN_UTILS_H
#define YUZU_TOOLS_TBLGEN_UTILS_TOKEN_UTILS_H

#include <llvm/ADT/StringRef.h>
#include <llvm/TableGen/Record.h>

#include <string>
#include <vector>

namespace yuzu::tools {

namespace detail {
/// Pull a `list<string>` field off a record into an owned `vector<string>`.
/// `getValueAsListOfStrings` returns `vector<StringRef>` whose lifetime is
/// tied to the record; copying upfront keeps the parsed structs self-contained.
inline std::vector<std::string> parseStringList(const llvm::Record *record,
                                                llvm::StringRef field) {
  const std::vector<llvm::StringRef> refs =
      record->getValueAsListOfStrings(field);
  std::vector<std::string> out;
  out.reserve(refs.size());
  for (llvm::StringRef ref : refs) {
    out.emplace_back(ref);
  }
  return out;
}
} // namespace detail

/// Unified in-memory mirror of a `Token` or `Regex` def from TokenBase.td.
/// `values` is populated for `Token` defs, `patterns` for `Regex` defs;
/// everything else is shared. The `is*` fields are the `Metadata` category
/// bits.
struct TokenInfo {
  std::string name;
  std::vector<std::string> values;
  std::vector<std::string> patterns;
  std::string callback;
  int priority;
  bool isSymbol;
  bool isPunctuation;
  bool isKeyword;
  bool isLiteral;
  bool isTrivia;
};

/// Parse either a `Token` or `Regex` def into a `TokenInfo`. Reads the
/// shared Metadata + Token/Regex fields, then populates `values` or
/// `patterns` based on which subclass the record is.
inline TokenInfo parseTokenInfo(const llvm::Record *record) {
  std::vector<std::string> values;
  std::vector<std::string> patterns;
  if (record->isSubClassOf("Token")) {
    values = detail::parseStringList(record, "Values");
  } else if (record->isSubClassOf("Regex")) {
    patterns = detail::parseStringList(record, "Patterns");
  }

  return TokenInfo{
      .name = record->getName().str(),
      .values = std::move(values),
      .patterns = std::move(patterns),
      .callback = record->getValueAsString("Callback").str(),
      .priority = static_cast<int>(record->getValueAsInt("Priority")),
      .isSymbol = record->getValueAsBit("IsSymbol"),
      .isPunctuation = record->getValueAsBit("IsPunctuation"),
      .isKeyword = record->getValueAsBit("IsKeyword"),
      .isLiteral = record->getValueAsBit("IsLiteral"),
      .isTrivia = record->getValueAsBit("IsTrivia"),
  };
}

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_UTILS_TOKEN_UTILS_H
