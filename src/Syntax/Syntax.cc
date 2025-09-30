#include "Syntax/Syntax.h"

#include "Syntax/SyntaxIterator.h"

namespace yuzu::syntax {
[[nodiscard]] SyntaxChildren SyntaxNode::getChildren() const {
  return SyntaxChildren(*this);
}

[[nodiscard]] SyntaxChildrenWithTokens
SyntaxNode::getChildrenWithTokens() const {
  return SyntaxChildrenWithTokens(*this);
}
} // namespace yuzu::syntax
