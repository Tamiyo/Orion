#ifndef SYNTAX_RGTREE_CURSOR_H_
#define SYNTAX_RGTREE_CURSOR_H_

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
        typename std::vector<GreenElement<SyntaxKind>>::const_iterator current)
        : current_(current) {}

    Iterator& operator++() {
      current_ += 1;
      return *this;
    }

    Iterator operator++(int) {
      current_ += 1;
      return *this;
    }

    // TODO(tamiyo) Implement this.
    // SyntaxElement<SyntaxKind> operator*() const {
    //   const GreenElement<SyntaxKind> child = *current_;
    //   return SyntaxElement<SyntaxKind>(
    //     child,
    //
    //     parent_.Offset() + child.Offset()
    //     );
    // }

    bool operator==(Iterator other) const { return current_ == other.current_; }

    bool operator!=(Iterator other) const { return !(*this == other); }

   private:
    typename std::vector<GreenElement<SyntaxKind>>::const_iterator current_;
  };

  explicit SyntaxElementChildren(const SyntaxNode<SyntaxKind>& parent)
      : parent_(parent) {}

  [[nodiscard]] Iterator Begin() const {
    return Iterator(parent_.Green().Children().begin());
  }
  [[nodiscard]] Iterator End() const {
    return Iterator(parent_.Green().Children().end());
  }

 private:
  const SyntaxNode<SyntaxKind> parent_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_RGTREE_CURSOR_H_
