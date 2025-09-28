#include "Syntax/Syntax.h"

#include "Syntax/SyntaxIterator.h"

namespace yuzu::syntax {
[[nodiscard]] SyntaxChildren SyntaxNode::getChildren() {
  return SyntaxChildren(this);
}

[[nodiscard]] SyntaxChildrenWithTokens SyntaxNode::getChildrenWithTokens() {
  return SyntaxChildrenWithTokens(this);
}
} // namespace yuzu::syntax
