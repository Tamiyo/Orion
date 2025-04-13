#include "syntax/parser/rgtree/green/green_writer.h"

#include <optional>
#include <stdexcept>
#include <string>

#include "syntax/parser/rgtree/green/green.h"
#include "syntax/syntax_kind.h"

namespace orion::syntax {
namespace {
std::u32string ToU32String(const size_t value) {
  std::string s = std::to_string(value);
  return {s.begin(), s.end()};
}
};  // namespace

GreenWriter& GreenWriter::Write(const GreenToken& token) noexcept {
  Indent();

  const size_t token_width = token.Source().size();
  string_ += SyntaxKindToString(token.Kind());
  string_ += U"@";
  string_ += ToU32String(width_);
  string_ += U"..";
  string_ += ToU32String(width_ + token_width);
  string_ += U" \"";
  string_ += token.Source();
  string_ += U"\"";
  string_ += U"\n";

  width_ += token_width;
  return *this;
}

GreenWriter& GreenWriter::Write(const GreenNode& node) noexcept {
  Indent();

  string_ += SyntaxKindToString(node.Kind());
  string_ += U"@";
  string_ += ToU32String(width_);
  string_ += U"..";
  string_ += ToU32String(width_ + node.Width());
  string_ += U"\n";

  IncreaseIndent();
  for (const GreenElement& element : node.Children()) {
    Write(element);
  }
  DecreaseIndent();

  return *this;
}

GreenWriter& GreenWriter::Write(const GreenElement& element) {
  if (const std::optional<GreenNode> node = element.TryGetNode();
      node.has_value()) {
    return Write(node.value());
  }

  if (const std::optional<GreenToken> token = element.TryGetToken();
      token.has_value()) {
    return Write(token.value());
  }

  throw std::invalid_argument("Unknown node type in GreenWriter.");
}

}  // namespace orion::syntax
