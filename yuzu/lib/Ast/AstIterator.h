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
template <typename T> class AstIterator final {
  static_assert(IsAstSubclass<T>::value,
                "T must be a subclass of AstNode<T> for some type T");

public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const T;
  using pointer = value_type *;
  using reference = value_type &;

  explicit AstIterator(syntax::SyntaxChildren::const_iterator It)
      : It_(std::move(It)) {}

  AstIterator() = delete;

  reference operator*() const { return *It_; }

  pointer operator->() const { return &It_; }

  AstIterator &operator++() {
    const auto End = syntax::SyntaxIterator(std::nullopt);
    while (++It_ != End) {
      const auto CastNode = T::cast(It_->getKind());
      if (CastNode.has_value()) {
        break;
      }
    }

    return *this;
  }

  AstIterator operator++(int) {
    AstIterator Tmp = *this;
    ++(*this);
    return Tmp;
  }

  AstIterator &operator--() {
    const auto End = syntax::SyntaxIterator(std::nullopt);
    while (--It_ != End) {
      const auto CastNode = AstNode<T>::cast(It_->getKind());
      if (CastNode.has_value()) {
        break;
      }
    }

    return *this;
  }

  AstIterator operator--(int) {
    AstIterator Tmp = *this;
    --(*this);
    return Tmp;
  }

  friend bool operator==(const AstIterator &A, const AstIterator &B) {
    return A.It_ == B.It_;
  }

  friend bool operator!=(const AstIterator &A, const AstIterator &B) {
    return !(A == B);
  }

private:
  syntax::SyntaxChildren::const_iterator It_;
};

template <typename N> class AstChildren final {
public:
  using const_iterator = AstIterator<N>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = typename const_iterator::value_type;

  explicit AstChildren(syntax::SyntaxChildren Children) : Children_(Children) {}
  AstChildren() = delete;

  const_iterator begin() const noexcept {
    return const_iterator(Children_.begin());
  }

  const_iterator end() const noexcept { return const_iterator(std::nullopt); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(Children_.rbegin());
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  const syntax::SyntaxChildren Children_;
};
} // namespace yuzu::ast

#endif // YUZU_AST_AST_ITERATOR_H
