#ifndef YUZU_SYNTAX_SYNTAX_PRINTER_H
#define YUZU_SYNTAX_SYNTAX_PRINTER_H

#include "yuzu/Syntax/Syntax.h"
#include "yuzu/Util/ErrorHandling.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/raw_ostream.h>

#include <cstddef>
#include <string>

namespace yuzu::syntax {

/// \brief Pretty-prints a typed syntax tree to a stream, using `asString`
/// to spell each `Kind`.
///
/// Output shape mirrors Rowan: each line is `KIND@START..END` for nodes and
/// `KIND@START..END "TEXT"` for tokens, two-space indent per depth, lines
/// joined with `\n` and no trailing newline. The named kind distinguishes
/// nodes from tokens, and the trailing quoted text is the additional signal
/// for tokens.
///
/// \code
///   BinaryExpr@0..3
///     Number@0..1 "3"
///     Minus@1..2 "-"
///     Number@2..3 "2"
/// \endcode
///
/// `Kind` must be paired with a free function
/// `std::string asString(Kind)` reachable by argument-dependent lookup
/// from the namespace of `Kind`. The TableGen-generated `SyntaxKind`
/// enums emit one for free; user-defined Kind enums must provide it.
template <typename Kind> class [[nodiscard]] SyntaxPrinter final {
public:
  /// \brief Construct a SyntaxPrinter that writes to `os`.
  ///
  /// The stream is borrowed for the lifetime of the printer; the caller
  /// retains ownership.
  ///
  /// \param os The stream to write to.
  explicit SyntaxPrinter(llvm::raw_ostream &os) : os(os) {}

  /// Deleted default constructor: a SyntaxPrinter must be bound to a stream.
  SyntaxPrinter() = delete;

  /// \brief Print a syntax node and its descendants.
  void print(const SyntaxNode<Kind> &node) { printNode(node, 0); }

  /// \brief Print a syntax token.
  void print(const SyntaxToken<Kind> &token) { printToken(token, 0); }

  /// \brief Print a syntax element (node or token).
  void print(const SyntaxElement<Kind> &element) {
    if (const SyntaxNode<Kind> *node = element.getIfNode()) {
      printNode(*node, 0);
      return;
    }

    if (const SyntaxToken<Kind> *token = element.getIfToken()) {
      printToken(*token, 0);
      return;
    }

    util::yuzu_unreachable();
  }

  /// \brief Print a syntax node into a freshly-allocated string.
  ///
  /// Convenience wrapper for tests, debugging, and ad-hoc inspection that
  /// don't want to manage their own stream.
  static std::string printToString(const SyntaxNode<Kind> &node) {
    std::string out;
    llvm::raw_string_ostream os(out);
    SyntaxPrinter(os).print(node);
    return out;
  }

  /// \brief Print a syntax token into a freshly-allocated string.
  static std::string printToString(const SyntaxToken<Kind> &token) {
    std::string out;
    llvm::raw_string_ostream os(out);
    SyntaxPrinter(os).print(token);
    return out;
  }

  /// \brief Print a syntax element into a freshly-allocated string.
  static std::string printToString(const SyntaxElement<Kind> &element) {
    std::string out;
    llvm::raw_string_ostream os(out);
    SyntaxPrinter(os).print(element);
    return out;
  }

private:
  void printNode(const SyntaxNode<Kind> &node, size_t indent) {
    const size_t offset = node.getOffset();
    os.indent(indent * 2);
    os << asString(node.getKind()) << '@' << offset << ".."
       << (offset + node.getGreen().getWidth());

    for (const SyntaxElement<Kind> &child : node.getChildrenWithTokens()) {
      os << '\n';
      if (const SyntaxNode<Kind> *childNode = child.getIfNode()) {
        printNode(*childNode, indent + 1);
      } else if (const SyntaxToken<Kind> *childToken = child.getIfToken()) {
        printToken(*childToken, indent + 1);
      } else {
        util::yuzu_unreachable();
      }
    }
  }

  void printToken(const SyntaxToken<Kind> &token, size_t indent) {
    const size_t offset = token.getOffset();
    os.indent(indent * 2);
    os << asString(token.getKind()) << '@' << offset << ".."
       << (offset + token.getGreen().getWidth()) << " \"";
    util::writeEscapedQuoted(os, token.getGreen().getSource());
    os << "\"";
  }

  llvm::raw_ostream &os;
};

} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_SYNTAX_PRINTER_H
