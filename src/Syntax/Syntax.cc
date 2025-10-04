#include "Syntax/Syntax.h"

#include "Syntax/SyntaxIterator.h"

namespace yuzu::syntax {
SyntaxChildrenWithoutTokens SyntaxNode::getChildren() const { return SyntaxChildrenWithoutTokens(this); }

SyntaxChildrenWithTokens SyntaxNode::getChildrenWithTokens() const {
  return SyntaxChildrenWithTokens(this);
}
} // namespace yuzu::syntax
