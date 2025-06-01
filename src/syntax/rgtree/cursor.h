#ifndef SYNTAX_RGTREE_CURSOR_H_
#define SYNTAX_RGTREE_CURSOR_H_

#include <cstdint>
#include <iterator>
#include <vector>

#include "syntax/rgtree/green.h"
#include "syntax/rgtree/syntax.h"

namespace yuzu::syntax {
template <typename SyntaxKind = uint16_t>
class SyntaxElementChildren {
 public:
  class Iterator
      : public std::iterator<std::forward_iterator_tag,
                             SyntaxElement<SyntaxKind>, std::ptrdiff_t,
                             const SyntaxElement<SyntaxKind>*,
                             const SyntaxElement<SyntaxKind>&> {
   public:
    explicit Iterator(
        const SyntaxNode<SyntaxKind> parent,
        typename std::vector<GreenElement<SyntaxKind>>::const_iterator current)
        : parent_(parent), next_(std::nullopt), current_(current) {}

    Iterator& operator++() {
      current_ += 1;
      return *this;
    }

    Iterator operator++(int) {
      current_ += 1;
      return *this;
    }

    SyntaxElement<SyntaxKind> operator*() const {
      const size_t index = current_->Index();
      const size_t offset = parent_.Offset() + current_.Width();
      return SyntaxNode<SyntaxKind>(index, offset, parent_, current_);
    }

    bool operator==(Iterator other) const { return current_ == other.current_; }
    bool operator!=(Iterator other) const { return !(*this == other); }

   private:
    const SyntaxNode<SyntaxKind> parent_;
    std::optional<SyntaxElement<SyntaxKind>> next_;
    typename std::vector<GreenElement<SyntaxKind>>::const_iterator current_;
  };

  explicit SyntaxElementChildren(const SyntaxNode<SyntaxKind>& parent)
      : parent_(parent) {}

  [[nodiscard]] Iterator Begin() const {
    return Iterator(parent_, parent_.Green().Children().begin());
  }
  [[nodiscard]] Iterator End() const {
    return Iterator(parent_, parent_.Green().Children().end());
  }

 private:
  const SyntaxNode<SyntaxKind> parent_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_RGTREE_CURSOR_H_
