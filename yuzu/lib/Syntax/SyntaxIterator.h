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

  explicit SyntaxIterator(std::optional<SyntaxNode> current)
      : current(current) {}

  SyntaxIterator() = delete;

  reference operator*() const { return current.value(); }

  pointer operator->() const { return &current.value(); }

  SyntaxIterator &operator++() {
    if (current.has_value()) {
      current = current->getNextSibling();
    }

    return *this;
  }

  SyntaxIterator operator++(int) {
    SyntaxIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  SyntaxIterator &operator--() {
    if (current.has_value()) {
      current = current->getPrevSibling();
    }

    return *this;
  }

  SyntaxIterator operator--(int) {
    SyntaxIterator tmp = *this;
    --(*this);
    return tmp;
  }

  friend bool operator==(const SyntaxIterator &a, const SyntaxIterator &b) {
    return a.current == b.current;
  }

  friend bool operator!=(const SyntaxIterator &a, const SyntaxIterator &b) {
    return !(a == b);
  }

private:
  std::optional<SyntaxNode> current;
};

class SyntaxIteratorWithTokens final {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxElement;
  using pointer = value_type *;
  using reference = value_type &;

  explicit SyntaxIteratorWithTokens(std::optional<SyntaxElement> current)
      : current(current) {}

  SyntaxIteratorWithTokens() = delete;

  reference operator*() const { return current.value(); }

  pointer operator->() const { return &current.value(); }

  SyntaxIteratorWithTokens &operator++() {
    if (current.has_value()) {
      current = current->getNextSiblingOrToken();
    }

    return *this;
  }

  SyntaxIteratorWithTokens operator++(int) {
    SyntaxIteratorWithTokens tmp = std::move(*this);
    ++(*this);
    return tmp;
  }

  SyntaxIteratorWithTokens &operator--() {
    if (current.has_value()) {
      current = current->getPrevSiblingOrToken();
    }

    return *this;
  }

  SyntaxIteratorWithTokens operator--(int) {
    SyntaxIteratorWithTokens tmp = std::move(*this);
    --(*this);
    return tmp;
  }

  friend bool operator==(const SyntaxIteratorWithTokens &a,
                         const SyntaxIteratorWithTokens &b) {
    return a.current == b.current;
  }

  friend bool operator!=(const SyntaxIteratorWithTokens &a,
                         const SyntaxIteratorWithTokens &b) {
    return !(a == b);
  }

private:
  std::optional<SyntaxElement> current;
};

class SyntaxChildren final {
public:
  using const_iterator = SyntaxIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = typename const_iterator::value_type;

  explicit SyntaxChildren(const SyntaxNode *node) : node(node) {}
  SyntaxChildren() = delete;

  const_iterator begin() const noexcept {
    return const_iterator(node->getFirstChild());
  }

  const_iterator end() const noexcept { return const_iterator(std::nullopt); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(const_iterator(node->getLastChild()));
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  const SyntaxNode *const node;
};

class SyntaxChildrenWithTokens final {
public:
  using const_iterator = SyntaxIteratorWithTokens;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = const_iterator::value_type;

  explicit SyntaxChildrenWithTokens(const SyntaxNode *node) : node(node) {}
  SyntaxChildrenWithTokens() = delete;

  const_iterator begin() const noexcept {
    return const_iterator(node->getFirstChildOrToken());
  }

  const_iterator end() const noexcept { return const_iterator(std::nullopt); }

  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(const_iterator(node->getLastChildOrToken()));
  }

  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  const SyntaxNode *const node;
};
} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_SYNTAX_ITERATOR_H
