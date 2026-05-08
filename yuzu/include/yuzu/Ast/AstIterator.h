#ifndef YUZU_AST_AST_ITERATOR_H
#define YUZU_AST_AST_ITERATOR_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Syntax/Api.h"

#include <cstddef>
#include <iterator>
#include <type_traits>
#include <utility>

namespace yuzu::ast {
// Required aliases for TableGen.
using SyntaxIterator = yuzu::syntax::api::SyntaxIterator<SyntaxKind>;

using SyntaxIteratorWithTokens =
    yuzu::syntax::api::SyntaxIteratorWithTokens<SyntaxKind>;

using SyntaxChildren = yuzu::syntax::api::SyntaxChildren<SyntaxKind>;

using SyntaxChildrenWithTokens =
    yuzu::syntax::api::SyntaxChildrenWithTokens<SyntaxKind>;

/// \brief Forward-only iterator over a SyntaxNode's child nodes, yielding
/// only those whose kind passes `Kind::isA`.
///
/// Wraps SyntaxChildren::const_iterator and skips children whose syntax
/// kind does not pass `Kind::isA`. Forward-only and by-value because the
/// underlying api iterator is forward-only (see syntax::api::SyntaxIterator).
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
    skipUntilIsA();
  }

  AstIterator() = delete;

  /// \brief Materialize the current child as Kind.
  /// \pre This iterator is not at end.
  Kind operator*() const { return Kind(*it); }

  /// \brief Pre-increment: advance past the current child and skip ahead
  /// to the next one whose kind passes Kind::isA.
  AstIterator &operator++() {
    ++it;
    skipUntilIsA();
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
  void skipUntilIsA() {
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
} // namespace yuzu::ast

#endif // YUZU_AST_AST_ITERATOR_H
