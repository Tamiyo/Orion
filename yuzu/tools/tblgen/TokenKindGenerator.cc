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
  if (count <= std::numeric_limits<std::uint8_t>::max()) {
    return "uint8_t";
  }

  if (count <= std::numeric_limits<std::uint16_t>::max()) {
    return "uint16_t";
  }

  if (count <= std::numeric_limits<std::uint32_t>::max()) {
    return "uint32_t";
  }

  return "uint64_t";
}

/// Sort defs by source position so the emitted enum follows include/file
/// order. Mirrors `SyntaxKindGenerator::byLoc` so the token block here lines
/// up numerically with the token block in `SyntaxKind`, making
/// `static_cast<syntax::SyntaxKind>(tokenKind)` a value-preserving cast.
bool byLoc(const llvm::Record *a, const llvm::Record *b) {
  return a->getLoc().front().getPointer() < b->getLoc().front().getPointer();
}

/// Emit the `TokenKind` enum with `TOKENS_FIRST` / `TOKENS_LAST` sentinels
/// bookending the metadata-derived tokens. The sentinels (and the in-between
/// values) are laid out in the same source order used by
/// `SyntaxKindGenerator`, so a `TokenKind::Plus` at value N matches
/// `SyntaxKind::Plus` at the same N.
///
/// `Error` lives outside the token range — it's the lexer's recovery
/// sentinel, not a real source token, and doesn't need to align with the
/// AST's `Error` (which sits in the System block).
void emitEnum(CodeFormatter &fmt, const std::vector<TokenInfo> &tokens) {
  // Tokens + 2 sentinels (TOKENS_FIRST/LAST) + 1 trailing `Error`.
  fmt.linef("enum class TokenKind : {0} {{",
            getUnderlyingType(tokens.size() + 3));
  {
    auto body = fmt.block();
    fmt.line("TOKENS_FIRST,");
    for (const TokenInfo &t : tokens) {
      fmt.linef("{0},", t.name);
    }
    fmt.line("TOKENS_LAST,");
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
  emit("Punctuation", [](const TokenInfo &t) { return t.isPunctuation; });
  emit("Keyword", [](const TokenInfo &t) { return t.isKeyword; });
  emit("Literal", [](const TokenInfo &t) { return t.isLiteral; });
  emit("Trivia", [](const TokenInfo &t) { return t.isTrivia; });
}

void emitAsString(CodeFormatter &fmt, const std::vector<TokenInfo> &tokens) {
  fmt.line("inline std::string asString(TokenKind kind) {");
  {
    auto body = fmt.block();
    fmt.line("switch (kind) {");
    fmt.line("case TokenKind::TOKENS_FIRST: return \"TOKENS_FIRST\";");
    for (const TokenInfo &t : tokens) {
      fmt.linef("case TokenKind::{0}: return \"{0}\";", t.name);
    }
    fmt.line("case TokenKind::TOKENS_LAST: return \"TOKENS_LAST\";");
    fmt.line("case TokenKind::Error: return \"Error\";");
    fmt.line("}");
    fmt.line("");
    fmt.line("util::yuzu_unreachable();");
  }
  fmt.line("}");
}

/// Prose form of a token kind, intended for diagnostic messages
/// ("expected <X>, found <Y>"). Picks the first non-empty source from:
///   1. the def's explicit `Display`,
///   2. `` `<value>` `` for a single-value `Token` (Equals → `` `=` ``),
///   3. the def name as a last resort.
/// The schema sentinels (`TOKENS_FIRST`/`TOKENS_LAST`) and `Error` aren't
/// real source tokens — calling this on them is a programming error.
void emitAsDisplayString(CodeFormatter &fmt,
                         const std::vector<TokenInfo> &tokens) {
  fmt.line("inline std::string asDisplayString(TokenKind kind) {");
  {
    auto body = fmt.block();
    fmt.line("switch (kind) {");
    for (const TokenInfo &t : tokens) {
      std::string display;
      if (!t.display.empty()) {
        display = t.display;
      } else if (t.values.size() == 1) {
        display = "`" + t.values[0] + "`";
      } else {
        display = t.name;
      }
      fmt.linef("case TokenKind::{0}: return \"{1}\";", t.name, display);
    }
    fmt.line("case TokenKind::TOKENS_FIRST:");
    fmt.line("case TokenKind::TOKENS_LAST:");
    fmt.line("case TokenKind::Error:");
    fmt.line("  util::yuzu_unreachable();");
    fmt.line("}");
    fmt.line("");
    fmt.line("util::yuzu_unreachable();");
  }
  fmt.line("}");
}

} // namespace

void TokenKindGenerator::generate(const llvm::RecordKeeper &records) {
  const std::string ns = findNamespace(records, "LexGrammar");

  // Sort the raw records by source location *before* parsing into TokenInfo
  // so the resulting numeric values line up with SyntaxKindGenerator's
  // matching token block.
  std::vector<const llvm::Record *> tokenRecords =
      records.getAllDerivedDefinitions("Metadata");
  std::sort(tokenRecords.begin(), tokenRecords.end(), byLoc);

  std::vector<TokenInfo> tokens;
  tokens.reserve(tokenRecords.size());
  for (const llvm::Record *r : tokenRecords) {
    tokens.push_back(parseTokenInfo(r));
  }

  fmt.linef("namespace {0} {{", ns);
  fmt.line("");
  emitEnum(fmt, tokens);
  emitPredicates(fmt, tokens);
  emitAsString(fmt, tokens);
  emitAsDisplayString(fmt, tokens);
  fmt.linef("} // namespace {0}", ns);
}

} // namespace yuzu::tools
