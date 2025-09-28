#ifndef SYNTAX_SYNTAX_ITERATOR_H
#define SYNTAX_SYNTAX_ITERATOR_H

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"
#include "Util/ErrorHandling.h"

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace yuzu::syntax {

struct NoFilter {
  bool operator()(const GreenElement &) const;
};

struct NodeOnlyFilter {
  bool operator()(const GreenElement &Element) const;
};

template <typename ValueType, typename FilterPredicate = NoFilter>
class SyntaxIterator {
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = ValueType;
  using pointer = value_type *;
  using reference = value_type &;

  explicit SyntaxIterator(std::vector<GreenElement>::const_iterator It,
                          SyntaxNode Parent,
                          std::vector<GreenElement>::const_iterator End,
                          FilterPredicate Filter = FilterPredicate{})
      : It_(It), Parent_(Parent), End_(End), Offset_(Parent.getOffset()),
        Filter_(Filter) {
    skipToValid();
  }

  value_type operator*() const { return createElement(); }

  SyntaxIterator &operator++() {
    Offset_ += (It_++)->getWidth();
    skipToValid();
    return *this;
  }

  SyntaxIterator operator++(int) {
    SyntaxIterator Tmp = *this;
    ++(*this);
    return Tmp;
  }

  friend bool operator==(const SyntaxIterator &A, const SyntaxIterator &B) {
    return A.It_ == B.It_ && A.Parent_ == B.Parent_;
  }

  friend bool operator!=(const SyntaxIterator &A, const SyntaxIterator &B) {
    return !(A == B);
  }

private:
  value_type createElement() const;

  inline void skipToValid() {
    while (It_ != End_ && !Filter_(*It_)) {
      Offset_ += (It_++)->getWidth();
    }
  }

  std::vector<GreenElement>::const_iterator It_;
  const SyntaxNode Parent_;
  std::vector<GreenElement>::const_iterator End_;
  size_t Offset_;
  FilterPredicate Filter_;
};

template <typename ValueType, typename FilterPredicate = NoFilter>
class SyntaxElementChildren {
public:
  using Iterator = SyntaxIterator<ValueType, FilterPredicate>;

  explicit SyntaxElementChildren(SyntaxNode Node,
                                 FilterPredicate Filter = FilterPredicate{})
      : Node_(Node), Filter_(Filter) {}

  SyntaxElementChildren() = delete;

  Iterator begin() {
    auto Begin = Node_.getGreen().getChildren().begin();
    auto End = Node_.getGreen().getChildren().end();
    return Iterator(Begin, Node_, End, Filter_);
  }

  Iterator end() {
    auto End = Node_.getGreen().getChildren().end();
    return Iterator(End, Node_, End, Filter_);
  }

private:
  SyntaxNode Node_;
  FilterPredicate Filter_;
};

using SyntaxChildren = SyntaxElementChildren<SyntaxNode, NodeOnlyFilter>;
using SyntaxChildrenWithTokens = SyntaxElementChildren<SyntaxElement, NoFilter>;

} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_ITERATOR_H
