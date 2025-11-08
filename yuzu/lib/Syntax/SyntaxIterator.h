#ifndef YUZU_SYNTAX_SYNTAX_ITERATOR_H
#define YUZU_SYNTAX_SYNTAX_ITERATOR_H

#include "yuzu/Syntax/Syntax.h"

#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>

namespace yuzu::syntax {
class SyntaxIterator final {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxNode;
  using pointer = value_type *;
  using reference = value_type &;

  explicit SyntaxIterator(std::optional<SyntaxNode> Current)
      : Current_(Current) {}

  SyntaxIterator() = delete;

  reference operator*() const { return Current_.value(); }

  pointer operator->() const { return &Current_.value(); }

  SyntaxIterator &operator++() {
    if (Current_.has_value()) {
      Current_ = Current_->getNextSibling();
    }

    return *this;
  }

  SyntaxIterator operator++(int) {
    SyntaxIterator Tmp = *this;
    ++(*this);
    return Tmp;
  }

  SyntaxIterator &operator--() {
    if (Current_.has_value()) {
      Current_ = Current_->getPrevSibling();
    }

    return *this;
  }

  SyntaxIterator operator--(int) {
    SyntaxIterator Tmp = *this;
    --(*this);
    return Tmp;
  }

  friend bool operator==(const SyntaxIterator &A, const SyntaxIterator &B) {
    return A.Current_ == B.Current_;
  }

  friend bool operator!=(const SyntaxIterator &A, const SyntaxIterator &B) {
    return !(A == B);
  }

private:
  std::optional<SyntaxNode> Current_;
};

class SyntaxIteratorWithTokens final {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxElement;
  using pointer = value_type *;
  using reference = value_type &;

  explicit SyntaxIteratorWithTokens(std::optional<SyntaxElement> Current)
      : Current_(Current) {}

  SyntaxIteratorWithTokens() = delete;

  reference operator*() const { return Current_.value(); }

  pointer operator->() const { return &Current_.value(); }

  SyntaxIteratorWithTokens &operator++() {
    if (Current_.has_value()) {
      Current_ = Current_->getNextSiblingOrToken();
    }

    return *this;
  }

  SyntaxIteratorWithTokens operator++(int) {
    SyntaxIteratorWithTokens Tmp = std::move(*this);
    ++(*this);
    return Tmp;
  }

  SyntaxIteratorWithTokens &operator--() {
    if (Current_.has_value()) {
      Current_ = Current_->getPrevSiblingOrToken();
    }

    return *this;
  }

  SyntaxIteratorWithTokens operator--(int) {
    SyntaxIteratorWithTokens Tmp = std::move(*this);
    --(*this);
    return Tmp;
  }

  friend bool operator==(const SyntaxIteratorWithTokens &A,
                         const SyntaxIteratorWithTokens &B) {
    return A.Current_ == B.Current_;
  }

  friend bool operator!=(const SyntaxIteratorWithTokens &A,
                         const SyntaxIteratorWithTokens &B) {
    return !(A == B);
  }

private:
  std::optional<SyntaxElement> Current_;
};

class SyntaxChildren final {
public:
  using const_iterator = SyntaxIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = typename const_iterator::value_type;

  explicit SyntaxChildren(const SyntaxNode *Node) : Node_(Node) {}
  SyntaxChildren() = delete;

  const_iterator begin() const noexcept {
    return const_iterator(Node_->getFirstChild());
  }

  const_iterator end() const noexcept { return const_iterator(std::nullopt); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(const_iterator(Node_->getLastChild()));
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  const SyntaxNode *const Node_;
};

class SyntaxChildrenWithTokens final {
public:
  using const_iterator = SyntaxIteratorWithTokens;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = const_iterator::value_type;

  explicit SyntaxChildrenWithTokens(const SyntaxNode *Node) : Node_(Node) {}
  SyntaxChildrenWithTokens() = delete;

  const_iterator begin() const noexcept {
    return const_iterator(Node_->getFirstChildOrToken());
  }

  const_iterator end() const noexcept { return const_iterator(std::nullopt); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(const_iterator(Node_->getLastChildOrToken()));
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  const SyntaxNode *const Node_;
};
} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_SYNTAX_ITERATOR_H
