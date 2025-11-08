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

  explicit GreenIterator(const GreenNode *Node, size_t Index)
      : Node_(Node), Index_(Index) {}

  GreenIterator() = delete;

  [[nodiscard]] reference operator*() const {
    return Node_->Data_->Children[Index_];
  }

  [[nodiscard]] pointer operator->() const {
    return &(Node_->Data_->Children[Index_]);
  }

  GreenIterator &operator++() {
    ++Index_;
    return *this;
  }

  GreenIterator &operator--() {
    --Index_;
    return *this;
  }

  GreenIterator operator++(int) {
    GreenIterator tmp = *this;
    ++Index_;
    return tmp;
  }

  GreenIterator operator--(int) {
    GreenIterator tmp = *this;
    --Index_;
    return tmp;
  }

  bool operator==(const GreenIterator &other) const {
    return Node_ == other.Node_ && Index_ == other.Index_;
  }

  bool operator!=(const GreenIterator &other) const {
    return !(*this == other);
  }

private:
  const GreenNode *Node_;
  size_t Index_;
};

class GreenChildren final {
public:
  using const_iterator = GreenIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = const_iterator::value_type;

  explicit GreenChildren(const GreenNode *Node) : Node_(Node) {}

  [[nodiscard]] size_t size() const { return Node_->getNumChildren(); };

  [[nodiscard]] const_iterator begin() const {
    return const_iterator(Node_, 0);
  }

  [[nodiscard]] const_iterator end() const {
    return const_iterator(Node_, Node_->getNumChildren());
  }

  [[nodiscard]] const_reverse_iterator rbegin() const {
    return const_reverse_iterator(end());
  }

  [[nodiscard]] const_reverse_iterator rend() const {
    return const_reverse_iterator(begin());
  }

private:
  const GreenNode *Node_;
};
} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_GREEN_GREEN_ITERATOR_H
