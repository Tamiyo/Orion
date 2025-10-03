#include "Syntax/Syntax.h"

#include "Syntax/SyntaxIterator.h"

namespace yuzu::syntax {
SyntaxChildren SyntaxNode::getChildren() const { return SyntaxChildren(*this); }

SyntaxChildrenWithTokens SyntaxNode::getChildrenWithTokens() const {
  return SyntaxChildrenWithTokens(*this);
}
} // namespace yuzu::syntax
