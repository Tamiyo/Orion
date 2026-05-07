#ifndef YUZU_AST_AST_H
#define YUZU_AST_AST_H

#include "yuzu/Ast/SyntaxKind.h"
#include "yuzu/Syntax/Syntax.h"
#include "yuzu/Syntax/SyntaxIterator.h" // IWYU pragma: keep — iterator types

#include <cstddef>
#include <optional>

namespace yuzu::ast {

/// \brief Find the n-th child of the given syntax node whose kind passes
/// `T::isA`, and return a typed view of it.
///
/// AST types are POD-shaped views over `syntax::SyntaxNode` (which is itself
/// a refcounted handle, cheap to copy). We return `std::optional<T>` by
/// value rather than `std::unique_ptr<T>` — no heap allocation per call.
///
/// \tparam T The AST view type to search for (must expose
///           `static bool isA(SyntaxKind)` and be aggregate-initializable
///           from a `syntax::SyntaxNode`).
/// \param parent The syntax node whose children to search.
/// \param n The zero-based index among matching children (default: 0).
/// \return The found child as `T`, or `std::nullopt` if not found.
template <typename T>
[[nodiscard]] inline std::optional<T>
child(const syntax::SyntaxNode &parent, std::size_t n = 0) {
  std::size_t count = 0;
  for (const auto &c : parent.getChildren()) {
    if (!T::isA(static_cast<SyntaxKind>(c.getKind()))) {
      continue;
    }
    if (count == n) {
      return T{c};
    }
    ++count;
  }
  return std::nullopt;
}

/// \brief Find the n-th token of `kind` among a syntax node's children
/// (including tokens).
[[nodiscard]] inline std::optional<syntax::SyntaxToken>
token(const syntax::SyntaxNode &parent, SyntaxKind kind,
      std::size_t n = 0) {
  std::size_t count = 0;
  for (const auto &c : parent.getChildrenWithTokens()) {
    if (static_cast<SyntaxKind>(c.getKind()) != kind) {
      continue;
    }
    if (count == n) {
      return c.getToken();
    }
    ++count;
  }
  return std::nullopt;
}

} // namespace yuzu::ast

// Generated AST view structs. Emits its own `namespace yuzu::ast { ... }`
// block, so it slots in after the runtime helpers above rather than inside
// their namespace.
#include "yuzu/Ast/Ast.h.inc" // IWYU pragma: export

#endif // YUZU_AST_AST_H
