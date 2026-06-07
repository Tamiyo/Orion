#include "TextMateGrammarGenerator.h"

#include "utils/TokenUtils.h"

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/StringSwitch.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/JSON.h>
#include <llvm/TableGen/Record.h>

#include <algorithm>
#include <string>
#include <vector>

namespace yuzu::tools {
namespace {
using llvm::json::Array;
using llvm::json::Object;
using llvm::json::Value;

// Values are taken by owned `std::string`: `json::Value` does not copy a
// `StringRef`, so passing a temporary would dangle.
Object include(std::string ref) { return Object{{"include", std::move(ref)}}; }

Object pattern(std::string name, std::string match) {
  return Object{{"name", std::move(name)}, {"match", std::move(match)}};
}

/// Regex-escape the literal characters of an operator/keyword value so it can
/// sit inside an alternation (`+` -> `\+`).
std::string escapeRegex(llvm::StringRef value) {
  static constexpr llvm::StringLiteral kSpecial = "\\^$.|?*+()[]{}/";
  std::string out;
  for (char c : value) {
    if (kSpecial.contains(c)) {
      out += '\\';
    }
    out += c;
  }
  return out;
}

std::string join(const std::vector<std::string> &parts, char separator) {
  std::string out;
  for (std::size_t i = 0; i < parts.size(); ++i) {
    if (i != 0) {
      out += separator;
    }
    out += parts[i];
  }
  return out;
}

/// The TextMate scope a literal token highlights as, keyed by its def name.
llvm::StringRef literalScope(llvm::StringRef name) {
  return llvm::StringSwitch<llvm::StringRef>(name)
      .Case("IntegerLiteral", "constant.numeric.integer.yuzu")
      .Case("FloatLiteral", "constant.numeric.float.yuzu")
      .Case("HexLiteral", "constant.numeric.hex.yuzu")
      .Case("BinaryLiteral", "constant.numeric.binary.yuzu")
      .Case("StringLiteral", "string.quoted.double.yuzu")
      .Case("RawStringLiteral", "string.quoted.double.raw.yuzu")
      .Default("constant.other.yuzu");
}

const TokenInfo *findToken(const std::vector<TokenInfo> &tokens,
                           llvm::StringRef name) {
  for (const TokenInfo &t : tokens) {
    if (t.name == name) {
      return &t;
    }
  }
  return nullptr;
}

/// Append a regex literal's `.td` patterns (verbatim) to `out`, scoped per
/// `literalScope`. The emission order of callers fixes match precedence
/// (e.g. hex/float before a bare integer).
void appendRegexLiteral(Array &out, const std::vector<TokenInfo> &tokens,
                        llvm::StringRef name) {
  const TokenInfo *token = findToken(tokens, name);
  if (token == nullptr) {
    return;
  }
  for (const std::string &p : token->patterns) {
    out.push_back(pattern(literalScope(name).str(), p));
  }
}
} // namespace

void TextMateGrammarGenerator::run(const llvm::RecordKeeper &records) {
  std::vector<TokenInfo> tokens;
  for (const llvm::Record *r : records.getAllDerivedDefinitions("Metadata")) {
    tokens.push_back(parseTokenInfo(r));
  }

  // Keyword and operator alternations, straight from the token values.
  std::vector<std::string> keywords;
  std::vector<std::string> operators;
  for (const TokenInfo &t : tokens) {
    if (t.isKeyword) {
      keywords.insert(keywords.end(), t.values.begin(), t.values.end());
    }
    if (t.isSymbol) {
      operators.insert(operators.end(), t.values.begin(), t.values.end());
    }
  }
  std::sort(keywords.begin(), keywords.end());
  // Longest operators first so multi-char operators win over their prefixes
  // (`**` before `*`, `->` before `-`, `<=` before `<`).
  std::sort(operators.begin(), operators.end(),
            [](const std::string &a, const std::string &b) {
              return a.size() > b.size();
            });
  std::vector<std::string> escapedOperators;
  escapedOperators.reserve(operators.size());
  for (const std::string &op : operators) {
    escapedOperators.push_back(escapeRegex(op));
  }

  const std::string keywordAlternation = "\\b(" + join(keywords, '|') + ")\\b";
  const std::string operatorAlternation =
      "(" + join(escapedOperators, '|') + ")";

  // String literals: raw before regular so `r"..."` isn't matched as `"..."`.
  Array stringPatterns;
  appendRegexLiteral(stringPatterns, tokens, "RawStringLiteral");
  appendRegexLiteral(stringPatterns, tokens, "StringLiteral");

  // Number literals: most-specific first (hex/binary/float before integer).
  Array numberPatterns;
  appendRegexLiteral(numberPatterns, tokens, "HexLiteral");
  appendRegexLiteral(numberPatterns, tokens, "BinaryLiteral");
  appendRegexLiteral(numberPatterns, tokens, "FloatLiteral");
  appendRegexLiteral(numberPatterns, tokens, "IntegerLiteral");

  Array keywordPatterns;
  keywordPatterns.push_back(pattern("keyword.control.yuzu", keywordAlternation));
  if (const TokenInfo *boolean = findToken(tokens, "BooleanLiteral");
      boolean != nullptr && !boolean->values.empty()) {
    keywordPatterns.push_back(pattern("constant.language.boolean.yuzu",
                                      "\\b(" + join(boolean->values, '|') +
                                          ")\\b"));
  }

  Object repository;
  repository["comments"] = Object{
      // The `//` line comment is lexer-special (the `Comment` token's regex
      // is empty), so it isn't derived from a token value.
      {"patterns",
       Array{pattern("comment.line.double-slash.yuzu", "//.*$")}}};
  repository["strings"] = Object{{"patterns", std::move(stringPatterns)}};
  repository["numbers"] = Object{{"patterns", std::move(numberPatterns)}};
  repository["function-declaration"] = Object{
      {"match", "\\b(fn)\\s+([A-Za-z_][A-Za-z0-9_]*)"},
      {"captures",
       Object{{"1", Object{{"name", "keyword.declaration.function.yuzu"}}},
              {"2", Object{{"name", "entity.name.function.yuzu"}}}}}};
  repository["keywords"] = Object{{"patterns", std::move(keywordPatterns)}};
  // Types appear only in positions the grammar can recognize structurally —
  // a type-parameter/argument list `[t, u]`, or after a `:` annotation or a
  // `->` return arrow. Matching by position (not capitalization) so lowercase
  // type parameters like `[t]` highlight too.
  Array typePatterns;
  typePatterns.push_back(
      Object{{"begin", "\\["},
             {"end", "\\]"},
             {"patterns", Array{pattern("entity.name.type.yuzu",
                                        "\\b[A-Za-z_][A-Za-z0-9_]*\\b")}}});
  typePatterns.push_back(
      pattern("entity.name.type.yuzu", "(?<=:)\\s*[A-Za-z_][A-Za-z0-9_]*"));
  typePatterns.push_back(
      pattern("entity.name.type.yuzu", "(?<=->)\\s*[A-Za-z_][A-Za-z0-9_]*"));
  repository["types"] = Object{{"patterns", std::move(typePatterns)}};
  repository["function-call"] =
      Object{{"name", "entity.name.function.call.yuzu"},
             {"match", "\\b([A-Za-z_][A-Za-z0-9_]*)(?=\\s*\\()"}};
  repository["operators"] =
      Object{{"patterns",
              Array{pattern("keyword.operator.yuzu", operatorAlternation)}}};

  Object root;
  root["information_for_contributors"] =
      Array{"Generated by `yuzu-tblgen -gen-textmate-grammar` from "
            "TokenKind.td.",
            "Run `make grammar` to regenerate; do not edit by hand."};
  root["$schema"] = "https://raw.githubusercontent.com/martinring/tmlanguage/"
                    "master/tmlanguage.json";
  root["name"] = "Yuzu";
  root["scopeName"] = "source.yuzu";
  root["patterns"] = Array{
      include("#comments"),  include("#strings"),       include("#numbers"),
      include("#function-declaration"), include("#keywords"),
      include("#types"),     include("#function-call"), include("#operators")};
  root["repository"] = std::move(repository);

  os << llvm::formatv("{0:2}", Value(std::move(root))) << "\n";
}

} // namespace yuzu::tools