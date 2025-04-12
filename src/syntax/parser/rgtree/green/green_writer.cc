#include "syntax/parser/rgtree/green/green_writer.h"

#include <optional>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/rgtree/green/green_node.h"
#include "syntax/parser/rgtree/green/green_token.h"
#include "syntax/syntax_kind.h"

namespace orion::syntax {
namespace {
std::u32string ToU32String(const size_t value) {
  std::string s = std::to_string(value);
  return std::u32string(s.begin(), s.end());
}
};  // namespace

GreenWriter& GreenWriter::Write(
    const GreenToken& token) noexcept {
  Indent();

  const size_t token_width = token.Source().size();
  char32_oss_ << token.Kind() << U"@" << ToU32String(width_) << U".."
              << ToU32String(width_ + token_width) << U" \"" << token.Source()
              << U"\"" << U"\n";

  width_ += token_width;
  return *this;
}

GreenWriter& GreenWriter::Write(const GreenNode& node) noexcept {
  Indent();

  char32_oss_ << node.Kind() << U"@" << ToU32String(width_) << U".."
              << ToU32String(width_ + node.Width()) << U"\n";

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

void GreenWriter::Print(
    std::basic_ostream<char32_t> char32_os) const noexcept {
  char32_os << char32_oss_.str();
}

std::u32string GreenWriter::AsU32String() const noexcept {
  return char32_oss_.str();
}

void GreenWriter::Clear() noexcept {
  char32_oss_.str(U"");
  char32_oss_.clear();
  indent_ = 0;
  width_ = 0;
}

void GreenWriter::Indent() {
  for (size_t i = 0; i < indent_ * indent_size_; i++) {
    char32_oss_ << U" ";
  }
}

}  // namespace orion::syntax
