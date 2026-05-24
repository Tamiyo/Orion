#ifndef YUZU_AST_AST_H
#define YUZU_AST_AST_H

#include "yuzu/Lexer/Range.h" // IWYU pragma: keep
#include "yuzu/Syntax/Syntax.h"
#include "yuzu/Util/ErrorHandling.h" // IWYU pragma: keep

#include <cstddef>
#include <cstdint> // IWYU pragma: keep
#include <iterator>
#include <optional>
#include <string>      // IWYU pragma: keep
#include <string_view> // IWYU pragma: keep
#include <type_traits>
#include <utility>

// Generated Syntax Kinds. Emits its own `namespace yuzu::ast { ... }`
// block, so it slots in after the runtime helpers above rather than inside
// their namespace.
#include "yuzu/Ast/SyntaxKind.h.inc" // IWYU pragma: export

namespace yuzu::ast {
// Forward-declared so AstIterator's `is_base_of_v<AstNode, Kind>` assertion
// can name it. The full definition lives in the generated `Ast.h.inc`
// emitted near the bottom of this header — by the time any AstIterator
// specialization is instantiated, AstNode is complete.
class AstNode;

// Required aliases for TableGen.
using SyntaxKind = yuzu::ast::SyntaxKind;
using SyntaxNode = yuzu::syntax::SyntaxNode<SyntaxKind>;
using SyntaxToken = yuzu::syntax::SyntaxToken<SyntaxKind>;
using SyntaxElement = yuzu::syntax::SyntaxElement<SyntaxKind>;
using SyntaxIterator = yuzu::syntax::SyntaxIterator<SyntaxKind>;
using SyntaxIteratorWithTokens =
    yuzu::syntax::SyntaxIteratorWithTokens<SyntaxKind>;
using SyntaxChildren = yuzu::syntax::SyntaxChildren<SyntaxKind>;
using SyntaxChildrenWithTokens =
    yuzu::syntax::SyntaxChildrenWithTokens<SyntaxKind>;

/// \brief Forward-only iterator over a SyntaxNode's child nodes, yielding
/// only those whose kind passes `Kind::isA`.
///
/// Wraps SyntaxChildren::const_iterator and skips children whose syntax
/// kind does not pass `Kind::isA`. Forward-only and by-value because the
/// underlying api iterator is forward-only (see syntax::SyntaxIterator).
template <typename Kind> class [[nodiscard]] AstIterator final {
  static_assert(std::is_base_of_v<AstNode, Kind>,
                "Kind must derive from yuzu::ast::AstNode");

public:
  using iterator_category = std::input_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = Kind;
  using pointer = void;
  using reference = Kind;

  /// \brief Construct an iterator at `it`, bounded by `end`.
  ///
  /// Both endpoints are required because the iterator advances past
  /// non-matching children on construction (and on each ++); knowing the
  /// end sentinel lets it stop without dereferencing past it.
  ///
  /// \param it Starting child position.
  /// \param end End sentinel from the same SyntaxChildren range.
  explicit AstIterator(SyntaxChildren::const_iterator it,
                       SyntaxChildren::const_iterator end)
      : it(std::move(it)), end(std::move(end)) {
    skipUntilIsInstance();
  }

  AstIterator() = delete;

  /// \brief Materialize the current child as Kind.
  /// \pre This iterator is not at end.
  Kind operator*() const { return Kind(*it); }

  /// \brief Pre-increment: advance past the current child and skip ahead
  /// to the next one whose kind passes Kind::isA.
  AstIterator &operator++() {
    ++it;
    skipUntilIsInstance();
    return *this;
  }

  /// \brief Post-increment.
  AstIterator operator++(int) {
    AstIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// \brief Equality comparison.
  friend bool operator==(const AstIterator &a, const AstIterator &b) {
    return a.it == b.it;
  }

  /// \brief Inequality comparison.
  friend bool operator!=(const AstIterator &a, const AstIterator &b) {
    return !(a == b);
  }

private:
  /// Advance `it` until either it reaches `end` or the current child's
  /// kind matches Kind::isA.
  void skipUntilIsInstance() {
    while (it != end && !Kind::isA((*it).getKind())) {
      ++it;
    }
  }

  SyntaxChildren::const_iterator it;
  SyntaxChildren::const_iterator end;
};

/// \brief Range over a SyntaxNode's children that cast to Kind.
template <typename Kind> class [[nodiscard]] AstChildren final {
public:
  using const_iterator = AstIterator<Kind>;
  using value_type = typename const_iterator::value_type;

  explicit AstChildren(SyntaxChildren children)
      : children(std::move(children)) {}

  AstChildren() = delete;

  const_iterator begin() const {
    return const_iterator(children.begin(), children.end());
  }

  const_iterator end() const {
    return const_iterator(children.end(), children.end());
  }

private:
  SyntaxChildren children;
};

/// \brief Find the n-th child of the given syntax node whose kind passes
/// `T::isA`, and return a typed view of it.
///
/// AST types are POD-shaped views over `SyntaxNode` (which is itself
/// a refcounted handle, cheap to copy). We return `std::optional<T>` by
/// value rather than `std::unique_ptr<T>` — no heap allocation per call.
///
/// \tparam T The AST view type to search for (must expose
///           `static bool isA(SyntaxKind)` and be aggregate-initializable
///           from a `SyntaxNode`).
/// \param parent The syntax node whose children to search.
/// \param n The zero-based index among matching children (default: 0).
/// \return The found child as `T`, or `std::nullopt` if not found.
template <typename T>
[[nodiscard]] inline std::optional<T> child(const SyntaxNode &parent,
                                            std::size_t n = 0) {
  std::size_t count = 0;
  for (const auto &c : parent.getChildren()) {
    if (!T::isA(c.getKind())) {
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
[[nodiscard]] inline std::optional<SyntaxToken>
token(const SyntaxNode &parent, SyntaxKind kind, std::size_t n = 0) {
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
