#ifndef YUZU_AST_AST_ITERATOR_H
#define YUZU_AST_AST_ITERATOR_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Syntax/SyntaxIterator.h"
#include "yuzu/lib/Ast/Ast.h"

#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>

namespace yuzu::ast {
template <typename T> class [[nodiscard]] AstIterator final {
  static_assert(IsAstSubclass<T>::value,
                "T must be a subclass of AstNode<T> for some type T");

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const T;
  using pointer = value_type *;
  using reference = value_type &;

  explicit AstIterator(syntax::SyntaxChildren::const_iterator it)
      : it(std::move(it)) {}

  AstIterator() = delete;

  reference operator*() const { return *it; }

  pointer operator->() const { return &it; }

  AstIterator &operator++() {
    const auto end = syntax::SyntaxIterator(std::nullopt);
    while (++it != end) {
      const auto castNode = T::cast(it->getKind());
      if (castNode.has_value()) {
        break;
      }
    }

    return *this;
  }

  AstIterator operator++(int) {
    AstIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  AstIterator &operator--() {
    const auto end = syntax::SyntaxIterator(std::nullopt);
    while (--it != end) {
      const auto castNode = AstNode<T>::cast(it->getKind());
      if (castNode.has_value()) {
        break;
      }
    }

    return *this;
  }

  AstIterator operator--(int) {
    AstIterator tmp = *this;
    --(*this);
    return tmp;
  }

  friend bool operator==(const AstIterator &a, const AstIterator &b) {
    return a.it == b.it;
  }

  friend bool operator!=(const AstIterator &a, const AstIterator &b) {
    return !(a == b);
  }

private:
  syntax::SyntaxChildren::const_iterator it;
};

template <typename N> class [[nodiscard]] AstChildren final {
public:
  using const_iterator = AstIterator<N>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = typename const_iterator::value_type;

  explicit AstChildren(syntax::SyntaxChildren children) : children(children) {}
  
  AstChildren() = delete;

  const_iterator begin() const noexcept {
    return const_iterator(children.begin());
  }

  const_iterator end() const noexcept { return const_iterator(std::nullopt); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(children.rbegin());
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  const syntax::SyntaxChildren children;
};
} // namespace yuzu::ast

#endif // YUZU_AST_AST_ITERATOR_H
