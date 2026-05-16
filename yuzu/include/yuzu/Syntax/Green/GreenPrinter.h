#ifndef YUZU_SYNTAX_GREEN_GREEN_PRINTER_H
#define YUZU_SYNTAX_GREEN_GREEN_PRINTER_H

#include "yuzu/Syntax/Green/Green.h"

#include <llvm/Support/raw_ostream.h>

#include <cstddef>
#include <string>

namespace yuzu::syntax {

/// \brief Pretty-prints a green tree to a stream.
///
/// The output is one line per element, indented two spaces per depth:
///
///   - `Node KIND@START..END` for non-terminals.
///   - `Token KIND@START..END "TEXT"` for terminals.
///
/// where:
///
///   - KIND is the numeric SyntaxKind (uint16_t). The green core is kind-
///     agnostic, so no symbolic name is emitted.
///   - START..END is the byte range the element spans, computed during
///     traversal from each child's relativeOffset (the green tree carries
///     widths but not absolute offsets).
///   - TEXT is the token's source with `\\`, `"`, `\n`, `\r`, and `\t`
///     escaped, encoded as UTF-8 from the underlying char32_t source.
///
/// Lines are joined with `\n`; the output does not end with a trailing
/// newline.
///
/// Example output for a node of kind 12 spanning `3-2`:
/// \code
///   Node 12@0..3
///     Token 2@0..1 "3"
///     Token 3@1..2 "-"
///     Token 2@2..3 "2"
/// \endcode
class [[nodiscard]] GreenPrinter final {
public:
  /// \brief Construct a GreenPrinter that writes to `os`.
  ///
  /// The stream is borrowed for the lifetime of the printer; the caller
  /// retains ownership.
  ///
  /// \param os The stream to write to.
  explicit GreenPrinter(llvm::raw_ostream &os) : os(os) {}

  /// Deleted default constructor: a GreenPrinter must be bound to a stream.
  GreenPrinter() = delete;

  /// \brief Print a green node and its descendants, rooted at offset 0.
  ///
  /// \param node The root green node to print.
  void print(const GreenNode &node);

  /// \brief Print a green token, rooted at offset 0.
  ///
  /// \param token The green token to print.
  void print(const GreenToken &token);

  /// \brief Print a green element (node or token), rooted at offset 0.
  ///
  /// \param element The green element to print.
  void print(const GreenElement &element);

  /// \brief Print a green node into a freshly-allocated string.
  ///
  /// Convenience wrapper for tests, debugging, and ad-hoc inspection that
  /// don't want to manage their own stream.
  ///
  /// \param node The green node to print.
  /// \return The formatted output as a UTF-8 string.
  static std::string printToString(const GreenNode &node);

  /// \brief Print a green token into a freshly-allocated string.
  ///
  /// \param token The green token to print.
  /// \return The formatted output as a UTF-8 string.
  static std::string printToString(const GreenToken &token);

  /// \brief Print a green element into a freshly-allocated string.
  ///
  /// \param element The green element to print.
  /// \return The formatted output as a UTF-8 string.
  static std::string printToString(const GreenElement &element);

private:
  /// \brief Recursively print a node, computing child offsets as we descend.
  void printNode(const GreenNode &node, size_t offset, size_t indent);

  /// \brief Print a single token line at the given offset and indent.
  void printToken(const GreenToken &token, size_t offset, size_t indent);

  llvm::raw_ostream &os;
};

} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_GREEN_GREEN_PRINTER_H
