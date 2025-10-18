#ifndef SYNTAX_SYNTAX_ITERATOR_H
#define SYNTAX_SYNTAX_ITERATOR_H

#include "Syntax/Syntax.h"

#include <iterator>
#include <optional>

namespace yuzu::syntax {
class SyntaxIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxNode;
  using pointer = const SyntaxNode *;
  using reference = const SyntaxNode &;

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

class SyntaxIteratorWithTokens {
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxElement;
  using pointer = const SyntaxElement *;
  using reference = const SyntaxElement &;

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
    SyntaxIteratorWithTokens Tmp = *this;
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
    SyntaxIteratorWithTokens Tmp = *this;
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

class SyntaxChildren {
public:
  using Iterator = SyntaxIterator;
  using ReverseIterator = std::reverse_iterator<Iterator>;

  explicit SyntaxChildren(const SyntaxNode *Node) : Node_(Node) {}
  SyntaxChildren() = delete;

  Iterator begin() const noexcept { return Iterator(Node_->getFirstChild()); }

  Iterator end() const noexcept { return Iterator(std::nullopt); }

  ReverseIterator rbegin() const noexcept {
    return ReverseIterator(Iterator(Node_->getLastChild()));
  }

  ReverseIterator rend() const noexcept {
    return ReverseIterator(Iterator(std::nullopt));
  }

private:
  const SyntaxNode *const Node_;
};

class SyntaxChildrenWithTokens {
public:
  using Iterator = SyntaxIteratorWithTokens;
  using ReverseIterator = std::reverse_iterator<Iterator>;

  explicit SyntaxChildrenWithTokens(const SyntaxNode *Node) : Node_(Node) {}
  SyntaxChildrenWithTokens() = delete;

  Iterator begin() const noexcept {
    return Iterator(Node_->getFirstChildOrToken());
  }

  Iterator end() const noexcept { return Iterator(std::nullopt); }

  ReverseIterator rbegin() const noexcept {
    return ReverseIterator(Iterator(Node_->getLastChildOrToken()));
  }

  ReverseIterator rend() const noexcept {
    return ReverseIterator(Iterator(std::nullopt));
  }

private:
  const SyntaxNode *const Node_;
};
} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_ITERATOR_H
