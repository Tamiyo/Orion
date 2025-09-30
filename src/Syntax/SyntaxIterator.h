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
                          std::vector<GreenElement>::const_iterator End,
                          SyntaxNode Parent,
                          FilterPredicate Filter = FilterPredicate{})
      : It_(It), End_(End), Parent_(Parent), Filter_(Filter),
        Offset_(Parent.getOffset()) {
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
  value_type createElement() const {
    if constexpr (std::is_same_v<ValueType, SyntaxNode>) {
      if (const std::optional<GreenNode> Node = It_->tryGetNode()) {
        return SyntaxNode(Offset_, &Parent_, Node.value());
      }
    }

    if constexpr (std::is_same_v<ValueType, SyntaxElement>) {
      if (const std::optional<GreenNode> Node = It_->tryGetNode()) {
        return SyntaxNode(Offset_, &Parent_, Node.value());
      }

      if (const std::optional<GreenToken> Token = It_->tryGetToken()) {
        return SyntaxToken(Offset_, &Parent_, Token.value());
      }
    }

    util::yuzu_unreachable();
  }

  inline void skipToValid() {
    while (It_ != End_ && !Filter_(*It_)) {
      Offset_ += (It_++)->getWidth();
    }
  }

  std::vector<GreenElement>::const_iterator It_;
  std::vector<GreenElement>::const_iterator End_;
  const SyntaxNode Parent_;
  FilterPredicate Filter_;
  size_t Offset_;
};

template <typename ValueType, typename FilterPredicate = NoFilter>
class SyntaxElementChildren {
public:
  using Iterator = SyntaxIterator<ValueType, FilterPredicate>;

  explicit SyntaxElementChildren(SyntaxNode Node,
                                 FilterPredicate Filter = FilterPredicate{})
      : Node_(Node), Filter_(Filter) {}

  SyntaxElementChildren() = delete;

  Iterator begin() const {
    auto Begin = Node_.getGreen().getChildren().begin();
    auto End = Node_.getGreen().getChildren().end();
    return Iterator(Begin, End, Node_, Filter_);
  }

  Iterator end() const {
    auto End = Node_.getGreen().getChildren().end();
    return Iterator(End, End, Node_, Filter_);
  }

private:
  SyntaxNode Node_;
  FilterPredicate Filter_;
};

class SyntaxChildren
    : public SyntaxElementChildren<SyntaxNode, NodeOnlyFilter> {
public:
  explicit SyntaxChildren(SyntaxNode Node)
      : SyntaxElementChildren<SyntaxNode, NodeOnlyFilter>(Node,
                                                          NodeOnlyFilter{}) {}

  SyntaxChildren() = delete;
};

class SyntaxChildrenWithTokens
    : public SyntaxElementChildren<SyntaxElement, NoFilter> {
public:
  explicit SyntaxChildrenWithTokens(SyntaxNode Node)
      : SyntaxElementChildren<SyntaxElement, NoFilter>(Node, NoFilter{}) {}

  SyntaxChildrenWithTokens() = delete;
};

} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_ITERATOR_H
