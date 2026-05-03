#include "TokenKindGenerator.h"

#include "utils/SchemaUtils.h"
#include "utils/TokenUtils.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/TableGen/Record.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>

namespace yuzu::tools {

namespace {

/// Smallest unsigned integer type that fits `count` distinct values. Used to
/// size the `TokenKind` enum's underlying type to the actual token
/// population.
llvm::StringRef getUnderlyingType(std::size_t count) {
  if (count <= std::numeric_limits<std::uint8_t>::max())
    return "uint8_t";
  if (count <= std::numeric_limits<std::uint16_t>::max())
    return "uint16_t";
  if (count <= std::numeric_limits<std::uint32_t>::max())
    return "uint32_t";
  return "uint64_t";
}

void emitEnum(CodeFormatter &fmt, const std::vector<TokenInfo> &tokens) {
  // +1 for the trailing `Error` enumerator.
  fmt.linef("enum class TokenKind : {0} {{",
            getUnderlyingType(tokens.size() + 1));
  {
    auto body = fmt.block();
    for (const TokenInfo &t : tokens) {
      fmt.linef("{0},", t.name);
    }
    fmt.line("Error,");
  }
  fmt.line("};");
  fmt.line("");
}

void emitPredicates(CodeFormatter &fmt, const std::vector<TokenInfo> &tokens) {
  auto emit = [&](llvm::StringRef name, auto select) {
    fmt.linef("inline bool is{0}(TokenKind kind) {{", name);
    {
      auto body = fmt.block();
      fmt.line("switch (kind) {");
      for (const TokenInfo &t : tokens) {
        if (select(t)) {
          fmt.linef("case TokenKind::{0}:", t.name);
        }
      }
      fmt.line("  return true;");
      fmt.line("default:");
      fmt.line("  return false;");
      fmt.line("}");
    }
    fmt.line("}");
    fmt.line("");
  };

  emit("Symbol", [](const TokenInfo &t) { return t.isSymbol; });
  emit("Keyword", [](const TokenInfo &t) { return t.isKeyword; });
  emit("Literal", [](const TokenInfo &t) { return t.isLiteral; });
  emit("Trivia", [](const TokenInfo &t) { return t.isTrivia; });
}

void emitAsString(CodeFormatter &fmt, const std::vector<TokenInfo> &tokens) {
  fmt.line("inline std::string asString(std::optional<TokenKind> kind) {");
  {
    auto body = fmt.block();
    fmt.line("if (!kind.has_value()) return \"None\";");
    fmt.line("");
    fmt.line("switch (*kind) {");
    for (const TokenInfo &t : tokens) {
      fmt.linef("case TokenKind::{0}: return \"{0}\";", t.name);
    }
    fmt.line("case TokenKind::Error: return \"Error\";");
    fmt.line("}");
    fmt.line("");
    fmt.line("util::yuzu_unreachable();");
  }
  fmt.line("}");
}

} // namespace

void TokenKindGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "LexGrammar");

  std::vector<TokenInfo> tokens;
  for (const llvm::Record *r : records.getAllDerivedDefinitions("Metadata")) {
    tokens.push_back(parseTokenInfo(r));
  }
  std::sort(
      tokens.begin(), tokens.end(),
      [](const TokenInfo &a, const TokenInfo &b) { return a.name < b.name; });

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");
  emitEnum(fmt, tokens);
  emitPredicates(fmt, tokens);
  emitAsString(fmt, tokens);
  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
