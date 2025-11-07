#include "yuzu/Syntax/Syntax.h"

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Green/GreenIterator.h"
#include "yuzu/Syntax/SyntaxIterator.h"
#include "yuzu/Util/ErrorHandling.h"

#include <cstddef>
#include <iterator>
#include <optional>
#include <variant>

namespace yuzu::syntax {
/// ===============
/// = SyntaxData =
/// ===============
std::optional<SyntaxNode> SyntaxData::getNextSibling() const noexcept {
  // Root nodes have no siblings.
  if (!Parent_) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent_->getGreen().getNode();

  // Ensure we don't iterate past the last child.
  if (Index_ + 1 >= Green.getNumChildren()) {
    return std::nullopt;
  }

  const auto Siblings = Green.getChildren();

  // Start after the current element to find the next sibling node.
  auto It = std::next(Siblings.begin(), Index_ + 1);
  const auto End = Siblings.end();

  // Find the next node, skipping any tokens.
  size_t SiblingIndex = Index_ + 1;
  for (; It != End; ++It, ++SiblingIndex) {
    const auto &Element = It->Element;
    if (std::holds_alternative<GreenNode>(Element)) {
      break;
    }
  }

  // No node siblings found after the current position.
  if (It == End) {
    return std::nullopt;
  }

  const auto &Element = It->Element;
  const size_t SiblingOffset = Parent_->getOffset() + It->RelativeOffset;
  return SyntaxNode(SiblingOffset, SiblingIndex, Parent_,
                    std::get<GreenNode>(Element));
}

std::optional<SyntaxElement>
SyntaxData::getNextSiblingOrToken() const noexcept {
  // Root nodes have no siblings.
  if (!Parent_) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent_->getGreen().getNode();

  // Ensure we don't iterate past the last child.
  if (Index_ + 1 >= Green.getNumChildren()) {
    return std::nullopt;
  }

  const auto Siblings = Green.getChildren();

  // Get the element immediately after the current position.
  auto It = std::next(Siblings.begin(), Index_ + 1);

  const auto &Element = It->Element;
  const size_t SiblingOffset = Parent_->getOffset() + It->RelativeOffset;
  const size_t SiblingIndex = Index_ + 1;

  if (std::holds_alternative<GreenNode>(Element)) {
    auto Node = SyntaxNode(SiblingOffset, SiblingIndex, Parent_,
                           std::get<GreenNode>(Element));

    return Node;
  }

  if (std::holds_alternative<GreenToken>(Element)) {
    auto Token = SyntaxToken(SiblingOffset, SiblingIndex, Parent_,
                             std::get<GreenToken>(Element));

    return Token;
  }

  util::yuzu_unreachable();
}

std::optional<SyntaxNode> SyntaxData::getPrevSibling() const noexcept {
  // Root nodes have no siblings.
  if (!Parent_) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent_->getGreen().getNode();

  // First child has no previous siblings.
  if (Index_ == 0) {
    return std::nullopt;
  }

  const auto Siblings = Green.getChildren();

  // Use reverse iterator to search backwards from current position.
  // std::next on rbegin() skips elements from the end, so we need to
  // calculate the offset to position just before the current element.
  auto It = std::next(Siblings.rbegin(), Green.getNumChildren() - Index_);
  const auto End = Siblings.rend();

  // Find the previous node, skipping any tokens.
  size_t SiblingIndex = Index_ - 1;
  for (; It != End; ++It, --SiblingIndex) {
    const auto &Element = It->Element;
    if (std::holds_alternative<GreenNode>(Element)) {
      break;
    }
  }

  // No node siblings found before the current position.
  if (It == End) {
    return std::nullopt;
  }

  const auto &Element = It->Element;
  const size_t SiblingOffset = Parent_->getOffset() + It->RelativeOffset;

  return SyntaxNode(SiblingOffset, SiblingIndex, Parent_,
                    std::get<GreenNode>(Element));
}

std::optional<SyntaxElement>
SyntaxData::getPrevSiblingOrToken() const noexcept {
  // Root nodes have no siblings.
  if (!Parent_) {
    return std::nullopt;
  }

  const GreenNode &Green = Parent_->getGreen().getNode();

  // First child has no previous siblings.
  if (Index_ == 0) {
    return std::nullopt;
  }

  const auto Siblings = Green.getChildren();

  // Get the element immediately before the current position using reverse
  // iterator.
  auto It = std::next(Siblings.rbegin(), Green.getNumChildren() - Index_);

  const auto &Element = It->Element;
  const size_t SiblingOffset = Parent_->getOffset() + It->RelativeOffset;
  const size_t SiblingIndex = Index_ + 1;

  if (std::holds_alternative<GreenNode>(Element)) {
    auto Node = SyntaxNode(SiblingOffset, SiblingIndex, Parent_,
                           std::get<GreenNode>(Element));
    return Node;
  }

  if (std::holds_alternative<GreenToken>(Element)) {
    auto Token = SyntaxToken(SiblingOffset, SiblingIndex, Parent_,
                             std::get<GreenToken>(Element));
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

std::optional<SyntaxNode> SyntaxNode::getFirstChild() const noexcept {
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
    const std::variant<GreenNode, GreenToken> Element = It->Element;
    if (std::holds_alternative<GreenNode>(Element)) {
      const size_t ChildOffset = getOffset() + It->RelativeOffset;
      return SyntaxNode(ChildOffset, ChildIndex, Data_,
                        std::get<GreenNode>(Element));
    }
  }

  return std::nullopt;
}

std::optional<SyntaxElement> SyntaxNode::getFirstChildOrToken() const noexcept {
  const GreenNode &Green = getGreen();

  // Empty nodes have no children.
  if (Green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren GreenChildren = Green.getChildren();
  const auto &Element = GreenChildren.begin()->Element;

  const size_t ChildOffset = getOffset();
  const size_t ChildIndex = 0;

  if (std::holds_alternative<GreenNode>(Element)) {
    auto Node = SyntaxNode(ChildOffset, ChildIndex, Data_,
                           std::get<GreenNode>(Element));
    return Node;
  }

  if (std::holds_alternative<GreenToken>(Element)) {
    auto Token = SyntaxToken(ChildOffset, ChildIndex, Data_,
                             std::get<GreenToken>(Element));
    return Token;
  }

  return std::nullopt;
}

std::optional<SyntaxNode> SyntaxNode::getLastChild() const noexcept {
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
    const std::variant<GreenNode, GreenToken> Element = It->Element;
    if (std::holds_alternative<GreenNode>(Element)) {
      const size_t ChildOffset = getOffset() + It->RelativeOffset;
      return SyntaxNode(ChildOffset, ChildIndex, Data_,
                        std::get<GreenNode>(Element));
    }
  }

  return std::nullopt;
}

std::optional<SyntaxElement> SyntaxNode::getLastChildOrToken() const noexcept {
  const GreenNode &Green = getGreen();

  // Empty nodes have no children.
  if (Green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren GreenChildren = Green.getChildren();
  const GreenChildren::const_reverse_iterator It = GreenChildren.rbegin();

  const size_t ChildOffset = getOffset() + It->RelativeOffset;
  const size_t ChildIndex = Green.getNumChildren() - 1;
  const std::variant<GreenNode, GreenToken> Element = It->Element;

  if (std::holds_alternative<GreenNode>(Element)) {
    auto Node = SyntaxNode(ChildOffset, ChildIndex, Data_,
                           std::get<GreenNode>(Element));
    return Node;
  }

  if (std::holds_alternative<GreenToken>(Element)) {
    auto Token = SyntaxToken(ChildOffset, ChildIndex, Data_,
                             std::get<GreenToken>(Element));
    return Token;
  }

  util::yuzu_unreachable();
}

std::optional<SyntaxNode> SyntaxNode::getNextSibling() const noexcept {
  return Data_->getNextSibling();
}

std::optional<SyntaxElement>
SyntaxNode::getNextSiblingOrToken() const noexcept {
  return Data_->getNextSiblingOrToken();
}

std::optional<SyntaxNode> SyntaxNode::getPrevSibling() const noexcept {
  return Data_->getPrevSibling();
}

std::optional<SyntaxElement>
SyntaxNode::getPrevSiblingOrToken() const noexcept {
  return Data_->getPrevSiblingOrToken();
}

/// ===============
/// = SyntaxToken =
/// ===============
std::optional<SyntaxNode> SyntaxToken::getNextSibling() const noexcept {
  return Data_->getNextSibling();
}

std::optional<SyntaxElement>
SyntaxToken::getNextSiblingOrToken() const noexcept {
  return Data_->getNextSiblingOrToken();
}

std::optional<SyntaxNode> SyntaxToken::getPrevSibling() const noexcept {
  return Data_->getPrevSibling();
}

std::optional<SyntaxElement>
SyntaxToken::getPrevSiblingOrToken() const noexcept {
  return Data_->getPrevSiblingOrToken();
}
} // namespace yuzu::syntax
