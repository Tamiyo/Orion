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

  const GreenNode &Green = Parent->getGreen();
  const GreenNode::Children Siblings = Green.getChildren();
  const GreenNode::Iterator End = Siblings.end();

  size_t SiblingIdx = 0;
  for (auto It = Siblings.begin(); It != End; ++It) {
    const GreenElement &Element = It->getElement();

    if (SiblingIdx <= Idx || !Element.isNode()) {
      SiblingIdx += 1;
      continue;
    }

    const size_t SiblingOffset = Parent->getOffset() + It->getRelativeOffset();
    return SyntaxNode(SiblingOffset, SiblingIdx, Parent, Element.getNode());
  }

  return std::nullopt;
}

std::optional<const SyntaxElement>
SyntaxData::getNextSiblingOrToken() const noexcept {
  if (!Parent) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent->getGreen();
  const GreenNode::Children Siblings = Green.getChildren();
  const GreenNode::Iterator End = Siblings.end();

  size_t SiblingIdx = 0;
  for (auto It = Siblings.begin(); It != End; ++It) {
    if (SiblingIdx <= Idx) {
      SiblingIdx += 1;
      continue;
    }

    const GreenElement &Element = It->getElement();
    const size_t SiblingOffset = Parent->getOffset() + It->getRelativeOffset();

    if (Element.isNode()) {
      auto Node =
          SyntaxNode(SiblingOffset, SiblingIdx, Parent, Element.getNode());
      return SyntaxElement(Node);
    }

    if (Element.isToken()) {
      auto Token =
          SyntaxToken(SiblingOffset, SiblingIdx, Parent, Element.getToken());
      return SyntaxElement(Token);
    }
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<const SyntaxNode>
SyntaxData::getPrevSibling() const noexcept {
  if (!Parent) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent->getGreen();
  const GreenNode::Children Siblings = Green.getChildren();
  const GreenNode::ReverseIterator End = Siblings.rend();

  size_t SiblingIdx = Green.getNumChildren() - 1;
  for (auto It = Siblings.rbegin(); It != End; ++It) {
    const GreenElement &Element = It->getElement();

    if (SiblingIdx >= Idx || !Element.isNode()) {
      SiblingIdx -= 1;
      continue;
    }

    const size_t SiblingOffset = Parent->getOffset() + It->getRelativeOffset();
    return SyntaxNode(SiblingOffset, SiblingIdx, Parent, Element.getNode());
  }

  return std::nullopt;
}

[[nodiscard]] std::optional<const SyntaxElement>
SyntaxData::getPrevSiblingOrToken() const noexcept {
  if (!Parent) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent->getGreen();
  const GreenNode::Children Siblings = Green.getChildren();
  const GreenNode::Iterator End = Siblings.end();

  size_t SiblingIdx = Green.getNumChildren() - 1;
  for (auto It = Siblings.begin(); It != End; ++It) {
    if (SiblingIdx >= Idx) {
      SiblingIdx -= 1;
      continue;
    }

    const GreenElement &Element = It->getElement();
    const size_t SiblingOffset = Parent->getOffset() + It->getRelativeOffset();

    if (Element.isNode()) {
      auto Node =
          SyntaxNode(SiblingOffset, SiblingIdx, Parent, Element.getNode());
      return SyntaxElement(Node);
    }

    if (Element.isToken()) {
      auto Token =
          SyntaxToken(SiblingOffset, SiblingIdx, Parent, Element.getToken());
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
  for (const GreenChild &Child : GreenChildren) {
    const GreenElement &Element = Child.getElement();

    if (Element.isNode()) {
      return SyntaxNode(getOffset() + Child.getRelativeOffset(), Idx, this,
                        Element.getNode());
    }

    Idx += 1;
  }

  return std::nullopt;
}

std::optional<const SyntaxElement>
SyntaxNode::getFirstChildOrToken() const noexcept {
  const GreenNode::Children GreenChildren = getGreen().getChildren();

  const size_t Idx = 0;
  for (const GreenChild &Child : GreenChildren) {
    const GreenElement &Element = Child.getElement();

    if (Element.isNode()) {
      auto Node = SyntaxNode(getOffset(), Idx, this, Element.getNode());
      return SyntaxElement(Node);
    }

    if (Element.isToken()) {
      auto Token = SyntaxToken(getOffset(), Idx, this, Element.getToken());
      return SyntaxElement(Token);
    }

    util::yuzu_unreachable();
  }

  return std::nullopt;
}

std::optional<const SyntaxNode> SyntaxNode::getLastChild() const noexcept {
  const GreenNode &Green = getGreen();
  const GreenNode::Children GreenChildren = Green.getChildren();
  const GreenNode::ReverseIterator End = GreenChildren.rend();

  size_t Idx = Green.getNumChildren() - 1;
  for (auto It = GreenChildren.rbegin(); It != End; It++) {
    const GreenElement &Element = It->getElement();

    if (Element.isNode()) {
      return SyntaxNode(getOffset() + It->getRelativeOffset(), Idx, this,
                        Element.getNode());
    }

    Idx -= 1;
  }

  return std::nullopt;
}

std::optional<const SyntaxElement>
SyntaxNode::getLastChildOrToken() const noexcept {
  const GreenNode &Green = getGreen();
  const GreenNode::Children GreenChildren = Green.getChildren();
  const GreenNode::ReverseIterator End = GreenChildren.rend();

  size_t Idx = Green.getNumChildren() - 1;
  for (auto It = GreenChildren.rbegin(); It != End; It++) {
    const GreenElement &Element = It->getElement();

    if (Element.isNode()) {
      auto Node = SyntaxNode(getOffset() + It->getRelativeOffset(), Idx, this,
                             Element.getNode());
      return SyntaxElement(Node);
    }

    if (Element.isToken()) {
      auto Token = SyntaxToken(getOffset() + It->getRelativeOffset(), Idx, this,
                               Element.getToken());
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

[[nodiscard]] std::optional<const SyntaxNode>
SyntaxNode::getPrevSibling() const noexcept {
  return Data_->getPrevSibling();
}

[[nodiscard]] std::optional<const SyntaxElement>
SyntaxNode::getPrevSiblingOrToken() const noexcept {
  return Data_->getPrevSiblingOrToken();
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

[[nodiscard]] std::optional<const SyntaxNode>
SyntaxToken::getPrevSibling() const noexcept {
  return Data_->getPrevSibling();
}

[[nodiscard]] std::optional<const SyntaxElement>
SyntaxToken::getPrevSiblingOrToken() const noexcept {
  return Data_->getPrevSiblingOrToken();
}
} // namespace yuzu::syntax
