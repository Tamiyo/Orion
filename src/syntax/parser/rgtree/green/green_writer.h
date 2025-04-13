#ifndef SYNTAX_PARSER_RGTREE_GREEN_GREEN_WRITER_H_
#define SYNTAX_PARSER_RGTREE_GREEN_GREEN_WRITER_H_

#include <sstream>
#include <string>

#include "syntax/parser/rgtree/green/green.h"

namespace orion::syntax {
/// \brief A utility class for serializing GreenElements, GreenNodes, and
/// GreenTokens
///        into a human-readable char32_t-based tree string representation.
///
/// This class is primarily intended for debugging or visualization of syntax
/// trees built using GreenElements. It implements a simple pretty-printing
/// format with indentation and width annotations.
class GreenWriter {
 public:
  /// \brief Constructs a writer with a configurable indentation size.
  /// \param indent_size Number of spaces per indentation level. Defaults to 2.
  explicit GreenWriter(const size_t indent_size = 2)
      : indent_size_(indent_size), indent_(0), width_(0) {}

  /// \brief Serializes a GreenElement into a u32string.
  /// \param element The element to write.
  /// \param indent_size The indentation level (optional).
  /// \returns A u32string representing the tree.
  [[nodiscard]] static std::u32string WriteAsU32String(
      const GreenElement& element, size_t indent_size = 2) {
    auto writer = GreenWriter(indent_size);
    return writer.Write(element).AsU32String();
  }

  /// \brief Serializes a GreenNode into a u32string.
  [[nodiscard]] static std::u32string WriteAsU32String(
      const GreenNode& node, size_t indent_size = 2) noexcept {
    auto writer = GreenWriter(indent_size);
    return writer.Write(node).AsU32String();
  }

  /// \brief Serializes a GreenToken into a u32string.
  [[nodiscard]] static std::u32string WriteAsU32String(
      const GreenToken& token, size_t indent_size = 2) noexcept {
    auto writer = GreenWriter(indent_size);
    return writer.Write(token).AsU32String();
  }

  /// \brief Writes a single GreenToken to the stream with width and source.
  GreenWriter& Write(const GreenToken& token) noexcept;

  /// \brief Recursively writes a GreenNode and its children.
  GreenWriter& Write(const GreenNode& node) noexcept;

  /// \brief Dispatches writing based on whether the element is a token or node.
  /// \throws std::invalid_argument if the element type is unrecognized.
  GreenWriter& Write(const GreenElement& element);

  /// \brief Returns the written representation as a u32string.
  [[nodiscard]] std::u32string AsU32String() const noexcept {
    return std::u32string(string_);
  }

  /// \brief Clears the internal stream and resets indentation/width counters.
  void Clear() noexcept {
    string_.clear();
    indent_ = 0;
    width_ = 0;
  }

 private:
  /// \brief Writes the current indentation to the stream.
  void Indent() {
    for (size_t i = 0; i < indent_ * indent_size_; i++) {
      string_ += U" ";
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

  /// Number of spaces per indent level.
  const size_t indent_size_;

  /// String for accumulating output.
  std::u32string string_;

  /// Current indentation depth.
  size_t indent_;

  /// Accumulated width offset for printing.
  size_t width_;
};

}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_RGTREE_GREEN_GREEN_WRITER_H_
