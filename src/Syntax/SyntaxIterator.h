#ifndef SYNTAX_SYNTAX_ITERATOR_H
#define SYNTAX_SYNTAX_ITERATOR_H

#include <memory>
#include <utility>
#include <vector>

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"
#include "Util/ErrorHandling.h"

namespace yuzu::syntax {
class SyntaxNodeChildren {
public:
  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = SyntaxNode;
    using pointer = value_type *;
    using reference = value_type &;

    explicit Iterator(std::vector<GreenElement>::const_iterator It,
                      SyntaxNode Parent)
        : It_(It), Parent_(Parent), Offset_(Parent.getOffset()) {}

    value_type operator*() const {
      if (const std::optional<GreenNode> Node = It_->tryGetNode()) {
        return SyntaxNode(Offset_ + Node->getWidth(), &Parent_, Node.value());
      }

      util::yuzu_unreachable();
    }

    Iterator &operator++() {
      Offset_ += (It_++)->getWidth();

      while (It_ != Parent_.getGreen().getChildren().end() && !It_->isNode())
        Offset_ += (It_++)->getWidth();

      return *this;
    }

    Iterator operator++(int) {
      Iterator Tmp = *this;
      ++(*this);
      return Tmp;
    }

    friend bool operator==(const Iterator &A, const Iterator &B) {
      return A.It_ == B.It_ && A.Parent_ == B.Parent_;
    };

    friend bool operator!=(const Iterator &A, const Iterator &B) {
      return !(A == B);
    };

  private:
    std::vector<GreenElement>::const_iterator It_;
    const SyntaxNode Parent_;
    size_t Offset_;
  };

  explicit SyntaxNodeChildren(SyntaxNode Node) : Node_(Node) {}

  SyntaxNodeChildren() = delete;

  Iterator begin() {
    std::vector<GreenElement>::const_iterator Begin =
        Node_.getGreen().getChildren().begin();

    std::vector<GreenElement>::const_iterator End =
        Node_.getGreen().getChildren().end();

    while (Begin != End && !(Begin->isNode()))
      Begin++;

    return Iterator(Begin, Node_);
  }

  Iterator end() {
    return Iterator(Node_.getGreen().getChildren().end(), Node_);
  }

private:
  SyntaxNode Node_;
};
} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_ITERATOR_H
