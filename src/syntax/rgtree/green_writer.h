#ifndef SYNTAX_RGTREE_GREEN_WRITER_H_
#define SYNTAX_RGTREE_GREEN_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>

#include "syntax/rgtree/green.h"

namespace yuzu::syntax {
/// \brief A utility class for serializing GreenElements, GreenNodes, and
/// GreenTokens
///        into a human-readable char32_t-based tree string representation.
///
/// This class is primarily intended for debugging or visualization of syntax
/// trees built using GreenElements. It implements a simple pretty-printing
/// format with indentation and width annotations.
template <typename SyntaxKind = uint16_t>
class GreenWriter {
 private:
  using GreenElement = GreenElement<SyntaxKind>;
  using GreenNode = GreenNode<SyntaxKind>;
  using GreenToken = GreenToken<SyntaxKind>;

 public:
  /// \brief Constructs a writer with a configurable indentation size.
  /// \param to_u32string A function to convert between SyntaxKind to an u32.
  /// \param indent_size Number of spaces per indentation level. Defaults to 2.
  explicit GreenWriter(
      const std::function<std::u32string_view(SyntaxKind)>& to_u32string,
      const size_t indent_size = 2)
      : syntax_kind_to_u32string_view_(to_u32string),
        indent_size_(indent_size),
        indent_(0),
        width_(0) {}

  /// \brief Serializes a GreenElement into an u32string.
  /// \param element The element to write.
  /// \param to_u32string A function to convert between SyntaxKind to an u32.
  /// \param indent_size The indentation level (optional).
  /// \returns An u32string representing the tree.
  [[nodiscard]] static std::u32string WriteAsU32String(
      const GreenElement& element,
      const std::function<std::u32string_view(SyntaxKind)>& to_u32string,
      size_t indent_size = 2) {
    auto writer = GreenWriter(to_u32string, indent_size);
    return writer.Write(element).AsU32String();
  }

  /// \brief Serializes a GreenNode into an u32string.
  [[nodiscard]] static std::u32string WriteAsU32String(
      const GreenNode& node,
      const std::function<std::u32string_view(SyntaxKind)>& to_u32string,
      size_t indent_size = 2) noexcept {
    auto writer = GreenWriter(to_u32string, indent_size);
    return writer.Write(node).AsU32String();
  }

  /// \brief Serializes a GreenToken into an u32string.
  [[nodiscard]] static std::u32string WriteAsU32String(
      const GreenToken& token,
      const std::function<std::u32string_view(SyntaxKind)>& to_u32string,
      size_t indent_size = 2) noexcept {
    auto writer = GreenWriter(to_u32string, indent_size);
    return writer.Write(token).AsU32String();
  }

  /// \brief Writes a single GreenToken to the stream with width and source.
  GreenWriter& Write(const GreenToken& token) noexcept {
    Indent();

    const size_t token_width = token.Source().size();
    u32string_ += syntax_kind_to_u32string_view_(token.Kind());
    u32string_ += U"@";
    u32string_ += ToU32String(width_);
    u32string_ += U"..";
    u32string_ += ToU32String(width_ + token_width);
    u32string_ += U" \"";
    u32string_ += token.Source();
    u32string_ += U"\"";
    u32string_ += U"\n";

    width_ += token_width;
    return *this;
  }

  /// \brief Recursively writes a GreenNode and its children.
  GreenWriter& Write(const GreenNode& node) noexcept {
    Indent();

    u32string_ += syntax_kind_to_u32string_view_(node.Kind());
    u32string_ += U"@";
    u32string_ += ToU32String(width_);
    u32string_ += U"..";
    u32string_ += ToU32String(width_ + node.Width());
    u32string_ += U"\n";

    IncreaseIndent();
    for (const GreenElement& element : node.Children()) {
      Write(element);
    }
    DecreaseIndent();

    return *this;
  }

  /// \brief Dispatches writing based on whether the element is a token or node.
  /// \throws std::invalid_argument if the element type is unrecognized.
  GreenWriter& Write(const GreenElement& element) {
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

  /// \brief Returns the written representation as a u32string.
  [[nodiscard]] std::u32string AsU32String() const noexcept {
    return {u32string_};
  }

  /// \brief Clears the internal stream and resets indentation/width counters.
  void Clear() noexcept {
    u32string_.clear();
    indent_ = 0;
    width_ = 0;
  }

 private:
  /// \brief Writes the current indentation to the stream.
  void Indent() {
    for (size_t i = 0; i < indent_ * indent_size_; i++) {
      u32string_ += U" ";
    }
  }

  /// \brief Increases the current indentation level by one.
  void IncreaseIndent() noexcept { indent_ += 1; }

  /// \brief Decreases the current indentation level by one.
  void DecreaseIndent() noexcept {
    if (indent_ > 0) {
      indent_ -= 1;
    }
  }

  std::u32string ToU32String(const size_t value) {
    std::string s = std::to_string(value);
    return {s.begin(), s.end()};
  }

  const std::function<std::u32string_view(SyntaxKind)>&
      syntax_kind_to_u32string_view_;
  const size_t indent_size_;
  std::u32string u32string_;
  size_t indent_;
  size_t width_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_RGTREE_GREEN_GREEN_WRITER_H_
