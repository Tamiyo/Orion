#include "yuzu/Syntax/Syntax.h"

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Green/GreenIterator.h"
#include "yuzu/Syntax/SyntaxIterator.h"
#include "yuzu/Util/ErrorHandling.h"

#include <cstddef>
#include <iterator>
#include <optional>

namespace yuzu::syntax {
/// ===============
/// = SyntaxData =
/// ===============
std::optional<const SyntaxNode> SyntaxData::getNextSibling() const noexcept {
  // Root nodes have no siblings.
  if (!Parent) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent->getGreen();

  // Ensure we don't iterate past the last child.
  if (Index + 1 >= Green.getNumChildren()) {
    return std::nullopt;
  }

  const auto Siblings = Green.getChildren();

  // Start after the current element to find the next sibling node.
  auto It = std::next(Siblings.begin(), Index + 1);
  const auto End = Siblings.end();

  // Find the next node, skipping any tokens.
  size_t SiblingIndex = Index + 1;
  for (; It != End; ++It, ++SiblingIndex) {
    const auto &Element = It->getElement();
    if (Element.isNode()) {
      break;
    }
  }

  // No node siblings found after the current position.
  if (It == End) {
    return std::nullopt;
  }

  const auto &Element = It->getElement();
  const size_t SiblingOffset = Parent->getOffset() + It->getRelativeOffset();
  return SyntaxNode(SiblingOffset, SiblingIndex, Parent, Element.getNode());
}

std::optional<const SyntaxElement>
SyntaxData::getNextSiblingOrToken() const noexcept {
  // Root nodes have no siblings.
  if (!Parent) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent->getGreen();

  // Ensure we don't iterate past the last child.
  if (Index + 1 >= Green.getNumChildren()) {
    return std::nullopt;
  }

  const auto Siblings = Green.getChildren();

  // Get the element immediately after the current position.
  auto It = std::next(Siblings.begin(), Index + 1);

  const auto &Element = It->getElement();
  const size_t SiblingOffset = Parent->getOffset() + It->getRelativeOffset();
  const size_t SiblingIndex = Index + 1;

  if (Element.isNode()) {
    auto Node =
        SyntaxNode(SiblingOffset, SiblingIndex, Parent, Element.getNode());

    return Node;
  }

  if (Element.isToken()) {
    auto Token =
        SyntaxToken(SiblingOffset, SiblingIndex, Parent, Element.getToken());

    return Token;
  }

  util::yuzu_unreachable();
}

std::optional<const SyntaxNode> SyntaxData::getPrevSibling() const noexcept {
  // Root nodes have no siblings.
  if (!Parent) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent->getGreen();

  // First child has no previous siblings.
  if (Index == 0) {
    return std::nullopt;
  }

  const auto Siblings = Green.getChildren();

  // Use reverse iterator to search backwards from current position.
  // std::next on rbegin() skips elements from the end, so we need to
  // calculate the offset to position just before the current element.
  auto It = std::next(Siblings.rbegin(), Green.getNumChildren() - Index);
  const auto End = Siblings.rend();

  // Find the previous node, skipping any tokens.
  size_t SiblingIndex = Index - 1;
  for (; It != End; ++It, --SiblingIndex) {
    const auto &Element = It->getElement();
    if (Element.isNode()) {
      break;
    }
  }

  // No node siblings found before the current position.
  if (It == End) {
    return std::nullopt;
  }

  const auto &Element = It->getElement();
  const size_t SiblingOffset = Parent->getOffset() + It->getRelativeOffset();

  return SyntaxNode(SiblingOffset, SiblingIndex, Parent, Element.getNode());
}

std::optional<const SyntaxElement>
SyntaxData::getPrevSiblingOrToken() const noexcept {
  // Root nodes have no siblings.
  if (!Parent) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent->getGreen();

  // First child has no previous siblings.
  if (Index == 0) {
    return std::nullopt;
  }

  const auto Siblings = Green.getChildren();

  // Get the element immediately before the current position using reverse
  // iterator.
  auto It = std::next(Siblings.rbegin(), Green.getNumChildren() - Index);

  const auto &Element = It->getElement();
  const size_t SiblingOffset = Parent->getOffset() + It->getRelativeOffset();
  const size_t SiblingIndex = Index + 1;

  if (Element.isNode()) {
    auto Node =
        SyntaxNode(SiblingOffset, SiblingIndex, Parent, Element.getNode());
    return Node;
  }

  if (Element.isToken()) {
    auto Token =
        SyntaxToken(SiblingOffset, SiblingIndex, Parent, Element.getToken());
    return Token;
  }

  util::yuzu_unreachable();
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
  const GreenNode &Green = getGreen();

  // Empty nodes have no children.
  if (Green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren GreenChildren = Green.getChildren();
  const auto End = GreenChildren.end();

  // Find the first node child, skipping any leading tokens.
  size_t ChildIndex = 0;
  for (auto It = GreenChildren.begin(); It != End; ++It, ++ChildIndex) {
    if (It->getElement().isNode()) {
      const size_t ChildOffset = getOffset() + It->getRelativeOffset();
      return SyntaxNode(ChildOffset, ChildIndex, this,
                        It->getElement().getNode());
    }
  }

  return std::nullopt;
}

std::optional<const SyntaxElement>
SyntaxNode::getFirstChildOrToken() const noexcept {
  const GreenNode &Green = getGreen();

  // Empty nodes have no children.
  if (Green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren GreenChildren = Green.getChildren();
  const auto &Element = GreenChildren.begin()->getElement();

  const size_t ChildOffset = getOffset();
  const size_t ChildIndex = 0;

  if (Element.isNode()) {
    auto Node = SyntaxNode(ChildOffset, ChildIndex, this, Element.getNode());
    return Node;
  }

  if (Element.isToken()) {
    auto Token = SyntaxToken(ChildOffset, ChildIndex, this, Element.getToken());
    return Token;
  }

  return std::nullopt;
}

std::optional<const SyntaxNode> SyntaxNode::getLastChild() const noexcept {
  const GreenNode &Green = getGreen();

  // Empty nodes have no children.
  if (Green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren GreenChildren = Green.getChildren();
  const auto End = GreenChildren.rend();

  // Find the last node child, skipping any trailing tokens.
  size_t ChildIndex = Green.getNumChildren() - 1;
  for (auto It = GreenChildren.rbegin(); It != End; ++It, --ChildIndex) {
    if (It->getElement().isNode()) {
      const size_t ChildOffset = getOffset() + It->getRelativeOffset();
      return SyntaxNode(ChildOffset, ChildIndex, this,
                        It->getElement().getNode());
    }
  }

  return std::nullopt;
}

std::optional<const SyntaxElement>
SyntaxNode::getLastChildOrToken() const noexcept {
  const GreenNode &Green = getGreen();

  // Empty nodes have no children.
  if (Green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren GreenChildren = Green.getChildren();
  const GreenChildren::const_reverse_iterator It = GreenChildren.rbegin();

  const size_t ChildOffset = getOffset() + It->getRelativeOffset();
  const size_t ChildIndex = Green.getNumChildren() - 1;

  if (It->getElement().isNode()) {
    auto Node =
        SyntaxNode(ChildOffset, ChildIndex, this, It->getElement().getNode());
    return Node;
  }

  if (It->getElement().isToken()) {
    auto Token =
        SyntaxToken(ChildOffset, ChildIndex, this, It->getElement().getToken());
    return Token;
  }

  util::yuzu_unreachable();
}

std::optional<const SyntaxNode> SyntaxNode::getNextSibling() const noexcept {
  return Data_->getNextSibling();
}

std::optional<const SyntaxElement>
SyntaxNode::getNextSiblingOrToken() const noexcept {
  return Data_->getNextSiblingOrToken();
}

std::optional<const SyntaxNode> SyntaxNode::getPrevSibling() const noexcept {
  return Data_->getPrevSibling();
}

std::optional<const SyntaxElement>
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

std::optional<const SyntaxNode> SyntaxToken::getPrevSibling() const noexcept {
  return Data_->getPrevSibling();
}

std::optional<const SyntaxElement>
SyntaxToken::getPrevSiblingOrToken() const noexcept {
  return Data_->getPrevSiblingOrToken();
}
} // namespace yuzu::syntax
