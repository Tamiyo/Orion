#ifndef SYNTAX_RGTREE_CURSOR_SYNTAX_NODE_CHILDREN_H_
#define SYNTAX_RGTREE_CURSOR_SYNTAX_NODE_CHILDREN_H_

#include <cstdint>
#include <iterator>
#include <vector>

#include "syntax/rgtree/green.h"
#include "syntax/rgtree/syntax.h"

namespace yuzu::syntax {
template <typename SyntaxKind = uint16_t>
class SyntaxNodeChildren {
 public:
  // Transition from SyntaxNode to the GreenNode.
  // Get the children() of the GreenNode.
  //
  class Iterator
      : public std::iterator<std::forward_iterator_tag, SyntaxNode<SyntaxKind>,
                             std::ptrdiff_t, const SyntaxNode<SyntaxKind>*,
                             const SyntaxNode<SyntaxKind>&> {
   public:
    bool operator==(Iterator other) const { return current_ == other.current_; }
    bool operator!=(Iterator other) const { return !(*this == other); }

   private:
    // The syntax node associated with this iterator.
    const SyntaxNode<SyntaxKind> syntax_;
  };
};
}  // namespace yuzu::syntax
#endif  // SYNTAX_RGTREE_CURSOR_SYNTAX_NODE_CHILDREN_H_
