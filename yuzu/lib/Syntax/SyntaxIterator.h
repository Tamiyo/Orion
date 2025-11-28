#ifndef YUZU_SYNTAX_SYNTAX_ITERATOR_H
#define YUZU_SYNTAX_SYNTAX_ITERATOR_H

#include "yuzu/Syntax/Syntax.h"

#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>

namespace yuzu::syntax {
/// \brief Bidirectional iterator for traversing sibling SyntaxNodes.
///
/// SyntaxIterator provides a standard C++ iterator interface for iterating
/// over sibling nodes in the syntax tree. It traverses only SyntaxNode
/// elements, skipping tokens. The iterator uses std::optional to represent
/// the end state.
class SyntaxIterator final {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxNode;
  using pointer = value_type *;
  using reference = value_type &;

  /// \brief Construct an iterator at a specific node.
  ///
  /// \param current The current SyntaxNode, or nullopt for end iterator.
  explicit SyntaxIterator(std::optional<SyntaxNode> current)
      : current(current) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxIterator() = delete;

  /// \brief Dereference operator.
  ///
  /// \return Reference to the current SyntaxNode.
  reference operator*() const { return current.value(); }

  /// \brief Member access operator.
  ///
  /// \return Pointer to the current SyntaxNode.
  pointer operator->() const { return &current.value(); }

  /// \brief Pre-increment operator.
  ///
  /// Advances the iterator to the next sibling node.
  ///
  /// \return Reference to this iterator.
  SyntaxIterator &operator++() {
    if (current.has_value()) {
      current = current->getNextSibling();
    }

    return *this;
  }

  /// \brief Post-increment operator.
  ///
  /// Advances the iterator to the next sibling node.
  ///
  /// \return Copy of the iterator before incrementing.
  SyntaxIterator operator++(int) {
    SyntaxIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// \brief Pre-decrement operator.
  ///
  /// Moves the iterator to the previous sibling node.
  ///
  /// \return Reference to this iterator.
  SyntaxIterator &operator--() {
    if (current.has_value()) {
      current = current->getPrevSibling();
    }

    return *this;
  }

  /// \brief Post-decrement operator.
  ///
  /// Moves the iterator to the previous sibling node.
  ///
  /// \return Copy of the iterator before decrementing.
  SyntaxIterator operator--(int) {
    SyntaxIterator tmp = *this;
    --(*this);
    return tmp;
  }

  /// \brief Equality comparison operator.
  ///
  /// \param a First iterator to compare.
  /// \param b Second iterator to compare.
  /// \return True if both iterators point to the same node.
  friend bool operator==(const SyntaxIterator &a, const SyntaxIterator &b) {
    return a.current == b.current;
  }

  /// \brief Inequality comparison operator.
  ///
  /// \param a First iterator to compare.
  /// \param b Second iterator to compare.
  /// \return True if the iterators point to different nodes.
  friend bool operator!=(const SyntaxIterator &a, const SyntaxIterator &b) {
    return !(a == b);
  }

private:
  /// The current node position, or nullopt if at end.
  std::optional<SyntaxNode> current;
};

/// \brief Bidirectional iterator for traversing sibling SyntaxElements.
///
/// SyntaxIteratorWithTokens provides a standard C++ iterator interface for
/// iterating over sibling elements in the syntax tree, including both nodes
/// and tokens. The iterator uses std::optional to represent the end state.
class SyntaxIteratorWithTokens final {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxElement;
  using pointer = value_type *;
  using reference = value_type &;

  /// \brief Construct an iterator at a specific element.
  ///
  /// \param current The current SyntaxElement, or nullopt for end iterator.
  explicit SyntaxIteratorWithTokens(std::optional<SyntaxElement> current)
      : current(current) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxIteratorWithTokens() = delete;

  /// \brief Dereference operator.
  ///
  /// \return Reference to the current SyntaxElement.
  reference operator*() const { return current.value(); }

  /// \brief Member access operator.
  ///
  /// \return Pointer to the current SyntaxElement.
  pointer operator->() const { return &current.value(); }

  /// \brief Pre-increment operator.
  ///
  /// Advances the iterator to the next sibling element (node or token).
  ///
  /// \return Reference to this iterator.
  SyntaxIteratorWithTokens &operator++() {
    if (current.has_value()) {
      current = current->getNextSiblingOrToken();
    }

    return *this;
  }

  /// \brief Post-increment operator.
  ///
  /// Advances the iterator to the next sibling element (node or token).
  ///
  /// \return Copy of the iterator before incrementing.
  SyntaxIteratorWithTokens operator++(int) {
    SyntaxIteratorWithTokens tmp = std::move(*this);
    ++(*this);
    return tmp;
  }

  /// \brief Pre-decrement operator.
  ///
  /// Moves the iterator to the previous sibling element (node or token).
  ///
  /// \return Reference to this iterator.
  SyntaxIteratorWithTokens &operator--() {
    if (current.has_value()) {
      current = current->getPrevSiblingOrToken();
    }

    return *this;
  }

  /// \brief Post-decrement operator.
  ///
  /// Moves the iterator to the previous sibling element (node or token).
  ///
  /// \return Copy of the iterator before decrementing.
  SyntaxIteratorWithTokens operator--(int) {
    SyntaxIteratorWithTokens tmp = std::move(*this);
    --(*this);
    return tmp;
  }

  /// \brief Equality comparison operator.
  ///
  /// \param a First iterator to compare.
  /// \param b Second iterator to compare.
  /// \return True if both iterators point to the same element.
  friend bool operator==(const SyntaxIteratorWithTokens &a,
                         const SyntaxIteratorWithTokens &b) {
    return a.current == b.current;
  }

  /// \brief Inequality comparison operator.
  ///
  /// \param a First iterator to compare.
  /// \param b Second iterator to compare.
  /// \return True if the iterators point to different elements.
  friend bool operator!=(const SyntaxIteratorWithTokens &a,
                         const SyntaxIteratorWithTokens &b) {
    return !(a == b);
  }

private:
  /// The current element position, or nullopt if at end.
  std::optional<SyntaxElement> current;
};

/// \brief Range wrapper for iterating over SyntaxNode children.
///
/// SyntaxChildren provides a standard C++ range interface for accessing
/// the child nodes of a SyntaxNode. It supports both forward and reverse
/// iteration using SyntaxIterator, traversing only node children and
/// skipping tokens.
class SyntaxChildren final {
public:
  using const_iterator = SyntaxIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = typename const_iterator::value_type;

  /// \brief Construct a range wrapper for a SyntaxNode.
  ///
  /// \param node The SyntaxNode whose children will be iterated.
  explicit SyntaxChildren(const SyntaxNode *node) : node(node) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxChildren() = delete;

  /// \brief Get an iterator to the first child node.
  ///
  /// \return Iterator pointing to the first child node.
  const_iterator begin() const noexcept {
    return const_iterator(node->getFirstChild());
  }

  /// \brief Get an iterator to past-the-end.
  ///
  /// \return Iterator pointing past the last child node.
  const_iterator end() const noexcept { return const_iterator(std::nullopt); }

  /// \brief Get a reverse iterator to the last child node.
  ///
  /// \return Reverse iterator pointing to the last child node.
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(const_iterator(node->getLastChild()));
  }

  /// \brief Get a reverse iterator to before-the-first.
  ///
  /// \return Reverse iterator pointing before the first child node.
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  /// The SyntaxNode whose children are being iterated.
  const SyntaxNode *const node;
};

/// \brief Range wrapper for iterating over SyntaxNode children and tokens.
///
/// SyntaxChildrenWithTokens provides a standard C++ range interface for
/// accessing all child elements of a SyntaxNode, including both nodes and
/// tokens. It supports both forward and reverse iteration using
/// SyntaxIteratorWithTokens.
class SyntaxChildrenWithTokens final {
public:
  using const_iterator = SyntaxIteratorWithTokens;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = const_iterator::value_type;

  /// \brief Construct a range wrapper for a SyntaxNode.
  ///
  /// \param node The SyntaxNode whose children will be iterated.
  explicit SyntaxChildrenWithTokens(const SyntaxNode *node) : node(node) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxChildrenWithTokens() = delete;

  /// \brief Get an iterator to the first child element.
  ///
  /// \return Iterator pointing to the first child element (node or token).
  const_iterator begin() const noexcept {
    return const_iterator(node->getFirstChildOrToken());
  }

  /// \brief Get an iterator to past-the-end.
  ///
  /// \return Iterator pointing past the last child element.
  const_iterator end() const noexcept { return const_iterator(std::nullopt); }

  /// \brief Get a reverse iterator to the last child element.
  ///
  /// \return Reverse iterator pointing to the last child element.
  const_reverse_iterator rbegin() const noexcept {
    return const_reverse_iterator(const_iterator(node->getLastChildOrToken()));
  }

  /// \brief Get a reverse iterator to before-the-first.
  ///
  /// \return Reverse iterator pointing before the first child element.
  const_reverse_iterator rend() const noexcept {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  /// The SyntaxNode whose children are being iterated.
  const SyntaxNode *const node;
};
} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_SYNTAX_ITERATOR_H
