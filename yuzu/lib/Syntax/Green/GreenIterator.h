#ifndef YUZU_SYNTAX_GREEN_GREEN_ITERATOR_H
#define YUZU_SYNTAX_GREEN_GREEN_ITERATOR_H

#include "yuzu/Syntax/Green/Green.h"

#include <cstddef>
#include <iterator>

namespace yuzu::syntax {
/// \brief Bidirectional iterator for traversing GreenNode children.
///
/// GreenIterator provides a standard C++ iterator interface for iterating
/// over the children of a GreenNode. It supports both forward and backward
/// iteration through the node's child elements.
class GreenIterator final {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const GreenChild;
  using pointer = value_type *;
  using reference = value_type &;

  /// \brief Construct an iterator for a GreenNode.
  ///
  /// \param node The GreenNode to iterate over.
  /// \param index The starting index position in the node's children.
  explicit GreenIterator(const GreenNode *node, size_t index)
      : node(node), index(index) {}

  /// Deleted default constructor to enforce proper initialization.
  GreenIterator() = delete;

  /// \brief Dereference operator.
  ///
  /// \return Reference to the current child element.
  [[nodiscard]] reference operator*() const {
    return node->data->children[index];
  }

  /// \brief Member access operator.
  ///
  /// \return Pointer to the current child element.
  [[nodiscard]] pointer operator->() const {
    return &(node->data->children[index]);
  }

  /// \brief Pre-increment operator.
  ///
  /// Advances the iterator to the next child.
  ///
  /// \return Reference to this iterator.
  GreenIterator &operator++() {
    ++index;
    return *this;
  }

  /// \brief Pre-decrement operator.
  ///
  /// Moves the iterator to the previous child.
  ///
  /// \return Reference to this iterator.
  GreenIterator &operator--() {
    --index;
    return *this;
  }

  /// \brief Post-increment operator.
  ///
  /// Advances the iterator to the next child.
  ///
  /// \return Copy of the iterator before incrementing.
  GreenIterator operator++(int) {
    GreenIterator tmp = *this;
    ++index;
    return tmp;
  }

  /// \brief Post-decrement operator.
  ///
  /// Moves the iterator to the previous child.
  ///
  /// \return Copy of the iterator before decrementing.
  GreenIterator operator--(int) {
    GreenIterator tmp = *this;
    --index;
    return tmp;
  }

  /// \brief Equality comparison operator.
  ///
  /// \param other The iterator to compare with.
  /// \return True if both iterators point to the same position.
  bool operator==(const GreenIterator &other) const {
    return node == other.node && index == other.index;
  }

  /// \brief Inequality comparison operator.
  ///
  /// \param other The iterator to compare with.
  /// \return True if the iterators point to different positions.
  bool operator!=(const GreenIterator &other) const {
    return !(*this == other);
  }

private:
  /// The GreenNode being iterated over.
  const GreenNode *node;

  /// Current index position in the node's children array.
  size_t index;
};

/// \brief Range wrapper for iterating over GreenNode children.
///
/// GreenChildren provides a standard C++ range interface for accessing
/// the children of a GreenNode. It supports both forward and reverse
/// iteration using the GreenIterator bidirectional iterator.
class GreenChildren final {
public:
  using const_iterator = GreenIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = const_iterator::value_type;

  /// \brief Construct a range wrapper for a GreenNode.
  ///
  /// \param node The GreenNode whose children will be iterated.
  explicit GreenChildren(const GreenNode *node) : node(node) {}

  /// \brief Get the number of children.
  ///
  /// \return The number of child elements in the node.
  [[nodiscard]] size_t size() const { return node->getNumChildren(); };

  /// \brief Get an iterator to the first child.
  ///
  /// \return Iterator pointing to the first child element.
  [[nodiscard]] const_iterator begin() const { return const_iterator(node, 0); }

  /// \brief Get an iterator to past-the-end.
  ///
  /// \return Iterator pointing past the last child element.
  [[nodiscard]] const_iterator end() const {
    return const_iterator(node, node->getNumChildren());
  }

  /// \brief Get a reverse iterator to the last child.
  ///
  /// \return Reverse iterator pointing to the last child element.
  [[nodiscard]] const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  /// \brief Get a reverse iterator to before-the-first.
  ///
  /// \return Reverse iterator pointing before the first child element.
  [[nodiscard]] const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

private:
  /// The GreenNode whose children are being iterated.
  const GreenNode *node;
};
} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_GREEN_GREEN_ITERATOR_H
