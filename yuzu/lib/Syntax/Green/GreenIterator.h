#ifndef YUZU_SYNTAX_GREEN_GREEN_ITERATOR_H
#define YUZU_SYNTAX_GREEN_GREEN_ITERATOR_H

#include "yuzu/Syntax/Green/Green.h"

#include <cstddef>
#include <iterator>

namespace yuzu::syntax {
class GreenIterator final {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const GreenChild;
  using pointer = value_type *;
  using reference = value_type &;

  explicit GreenIterator(const GreenNode *node, size_t index)
      : node(node), index(index) {}

  GreenIterator() = delete;

  [[nodiscard]] reference operator*() const {
    return node->data->children[index];
  }

  [[nodiscard]] pointer operator->() const {
    return &(node->data->children[index]);
  }

  GreenIterator &operator++() {
    ++index;
    return *this;
  }

  GreenIterator &operator--() {
    --index;
    return *this;
  }

  GreenIterator operator++(int) {
    GreenIterator tmp = *this;
    ++index;
    return tmp;
  }

  GreenIterator operator--(int) {
    GreenIterator tmp = *this;
    --index;
    return tmp;
  }

  bool operator==(const GreenIterator &other) const {
    return node == other.node && index == other.index;
  }

  bool operator!=(const GreenIterator &other) const {
    return !(*this == other);
  }

private:
  const GreenNode *node;
  size_t index;
};

class GreenChildren final {
public:
  using const_iterator = GreenIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = const_iterator::value_type;

  explicit GreenChildren(const GreenNode *node) : node(node) {}

  [[nodiscard]] size_t size() const { return node->getNumChildren(); };

  [[nodiscard]] const_iterator begin() const { return const_iterator(node, 0); }

  [[nodiscard]] const_iterator end() const {
    return const_iterator(node, node->getNumChildren());
  }

  [[nodiscard]] const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  [[nodiscard]] const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

private:
  const GreenNode *node;
};
} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_GREEN_GREEN_ITERATOR_H
