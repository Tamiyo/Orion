#include "Syntax/Syntax.h"

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"
#include "Syntax/SyntaxIterator.h"
#include "Util/ErrorHandling.h"

#include <optional>

namespace yuzu::syntax {
/// ===============
/// = SyntaxData =
/// ===============
std::optional<const SyntaxNode> SyntaxData::getNextSibling() const noexcept {
  if (!Parent) {
    return std::nullopt;
  }

  const GreenNode::Children Siblings = Parent->getGreen().getChildren();
  const GreenNode::Iterator End = Siblings.end();

  size_t SiblingIdx = 0;
  size_t SiblingOffset = Parent->getOffset();

  for (auto It = Siblings.begin(); It != End; ++It) {
    if (SiblingIdx <= Idx || !It->isNode()) {
      SiblingOffset += It->getWidth();
      SiblingIdx += 1;
      continue;
    }

    const GreenElement &Sibling = *It;

    return SyntaxNode(SiblingOffset, SiblingIdx, Parent, Sibling.getNode());
  }

  return std::nullopt;
}

std::optional<const SyntaxElement>
SyntaxData::getNextSiblingOrToken() const noexcept {
  if (!Parent) {
    return std::nullopt;
  }

  const GreenNode::Children Siblings = Parent->getGreen().getChildren();
  const GreenNode::Iterator End = Siblings.end();

  size_t SiblingIdx = 0;
  size_t SiblingOffset = Parent->getOffset();

  // Computing this constantly is tiring... Rowan has a method by
  // pre-calculating the relative offsets of children of GreenNodes that is
  // better.
  for (auto It = Siblings.begin(); It != End; ++It) {
    if (SiblingIdx <= Idx) {
      SiblingOffset += It->getWidth();
      SiblingIdx += 1;
      continue;
    }

    const GreenElement &Sibling = *It;

    if (Sibling.isNode()) {
      auto Node =
          SyntaxNode(SiblingOffset, SiblingIdx, Parent, Sibling.getNode());
      return SyntaxElement(Node);
    }

    if (Sibling.isToken()) {
      auto Token =
          SyntaxToken(SiblingOffset, SiblingIdx, Parent, Sibling.getToken());
      return SyntaxElement(Token);
    }
  }

  return std::nullopt;
}

/// ==============
/// = SyntaxNode =
/// ==============
SyntaxChildren SyntaxNode::getChildren() const noexcept {
  return SyntaxChildren(this);
}

SyntaxChildrenWithTokens SyntaxNode::getChildrenWithTokens() const noexcept {
  return SyntaxChildrenWithTokens(this);
}

std::optional<const SyntaxNode> SyntaxNode::getFirstChild() const noexcept {
  const GreenNode::Children GreenChildren = getGreen().getChildren();

  size_t Idx = 0;
  size_t Offset = getOffset();
  for (const auto &Child : GreenChildren) {
    if (Child.isNode()) {
      return SyntaxNode(Offset, Idx, this, Child.getNode());
    }

    Offset += Child.getWidth();
    Idx += 1;
  }

  return std::nullopt;
}

std::optional<const SyntaxElement>
SyntaxNode::getFirstChildOrToken() const noexcept {
  const GreenNode::Children GreenChildren = getGreen().getChildren();

  const size_t Idx = 0;
  for (const auto &Child : GreenChildren) {
    if (Child.isNode()) {
      auto Node = SyntaxNode(getOffset(), Idx, this, Child.getNode());
      return SyntaxElement(Node);
    }

    if (Child.isToken()) {
      auto Token = SyntaxToken(getOffset(), Idx, this, Child.getToken());
      return SyntaxElement(Token);
    }

    util::yuzu_unreachable();
  }

  return std::nullopt;
}

std::optional<const SyntaxNode> SyntaxNode::getNextSibling() const noexcept {
  return Data_->getNextSibling();
}

std::optional<const SyntaxElement>
SyntaxNode::getNextSiblingOrToken() const noexcept {
  return Data_->getNextSiblingOrToken();
}

/// ===============
/// = SyntaxToken =
/// ===============
std::optional<const SyntaxNode> SyntaxToken::getNextSibling() const noexcept {
  return Data_->getNextSibling();
}

std::optional<const SyntaxElement>
SyntaxToken::getNextSiblingOrToken() const noexcept {
  return Data_->getNextSiblingOrToken();
}
} // namespace yuzu::syntax
