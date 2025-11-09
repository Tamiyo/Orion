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
  if (!parent) {
    return std::nullopt;
  }

  const GreenNode &green = parent->getGreen().getNode();

  // Ensure we don't iterate past the last child.
  if (index + 1 >= green.getNumChildren()) {
    return std::nullopt;
  }

  const auto siblings = green.getChildren();

  // Start after the current element to find the next sibling node.
  auto it = std::next(siblings.begin(), index + 1);
  const auto end = siblings.end();

  // Find the next node, skipping any tokens.
  size_t siblingIndex = index + 1;
  for (; it != end; ++it, ++siblingIndex) {
    const auto &element = it->element;
    if (std::holds_alternative<GreenNode>(element)) {
      break;
    }
  }

  // No node siblings found after the current position.
  if (it == end) {
    return std::nullopt;
  }

  const auto &element = it->element;
  const size_t siblingOffset = parent->getOffset() + it->relativeOffset;
  return SyntaxNode(siblingOffset, siblingIndex, parent,
                    std::get<GreenNode>(element));
}

std::optional<SyntaxElement>
SyntaxData::getNextSiblingOrToken() const noexcept {
  // Root nodes have no siblings.
  if (!parent) {
    return std::nullopt;
  }

  const GreenNode &green = parent->getGreen().getNode();

  // Ensure we don't iterate past the last child.
  if (index + 1 >= green.getNumChildren()) {
    return std::nullopt;
  }

  const auto siblings = green.getChildren();

  // Get the element immediately after the current position.
  auto it = std::next(siblings.begin(), index + 1);

  const auto &element = it->element;
  const size_t siblingOffset = parent->getOffset() + it->relativeOffset;
  const size_t siblingIndex = index + 1;

  if (std::holds_alternative<GreenNode>(element)) {
    auto node = SyntaxNode(siblingOffset, siblingIndex, parent,
                           std::get<GreenNode>(element));

    return node;
  }

  if (std::holds_alternative<GreenToken>(element)) {
    auto token = SyntaxToken(siblingOffset, siblingIndex, parent,
                             std::get<GreenToken>(element));

    return token;
  }

  util::yuzu_unreachable();
}

std::optional<SyntaxNode> SyntaxData::getPrevSibling() const noexcept {
  // Root nodes have no siblings.
  if (!parent) {
    return std::nullopt;
  }

  const GreenNode &green = parent->getGreen().getNode();

  // First child has no previous siblings.
  if (index == 0) {
    return std::nullopt;
  }

  const auto siblings = green.getChildren();

  // Use reverse iterator to search backwards from current position.
  // std::next on rbegin() skips elements from the end, so we need to
  // calculate the offset to position just before the current element.
  auto it = std::next(siblings.rbegin(), green.getNumChildren() - index);
  const auto end = siblings.rend();

  // Find the previous node, skipping any tokens.
  size_t siblingIndex = index - 1;
  for (; it != end; ++it, --siblingIndex) {
    const auto &element = it->element;
    if (std::holds_alternative<GreenNode>(element)) {
      break;
    }
  }

  // No node siblings found before the current position.
  if (it == end) {
    return std::nullopt;
  }

  const auto &element = it->element;
  const size_t siblingOffset = parent->getOffset() + it->relativeOffset;

  return SyntaxNode(siblingOffset, siblingIndex, parent,
                    std::get<GreenNode>(element));
}

std::optional<SyntaxElement>
SyntaxData::getPrevSiblingOrToken() const noexcept {
  // Root nodes have no siblings.
  if (!parent) {
    return std::nullopt;
  }

  const GreenNode &green = parent->getGreen().getNode();

  // First child has no previous siblings.
  if (index == 0) {
    return std::nullopt;
  }

  const auto siblings = green.getChildren();

  // Get the element immediately before the current position using reverse
  // iterator.
  auto it = std::next(siblings.rbegin(), green.getNumChildren() - index);

  const auto &element = it->element;
  const size_t siblingOffset = parent->getOffset() + it->relativeOffset;
  const size_t siblingIndex = index + 1;

  if (std::holds_alternative<GreenNode>(element)) {
    auto node = SyntaxNode(siblingOffset, siblingIndex, parent,
                           std::get<GreenNode>(element));
    return node;
  }

  if (std::holds_alternative<GreenToken>(element)) {
    auto token = SyntaxToken(siblingOffset, siblingIndex, parent,
                             std::get<GreenToken>(element));
    return token;
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
  const GreenNode &green = getGreen();

  // Empty nodes have no children.
  if (green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren greenChildren = green.getChildren();
  const auto end = greenChildren.end();

  // Find the first node child, skipping any leading tokens.
  size_t childIndex = 0;
  for (auto it = greenChildren.begin(); it != end; ++it, ++childIndex) {
    const std::variant<GreenNode, GreenToken> element = it->element;
    if (std::holds_alternative<GreenNode>(element)) {
      const size_t childOffset = getOffset() + it->relativeOffset;
      return SyntaxNode(childOffset, childIndex, data,
                        std::get<GreenNode>(element));
    }
  }

  return std::nullopt;
}

std::optional<SyntaxElement> SyntaxNode::getFirstChildOrToken() const noexcept {
  const GreenNode &green = getGreen();

  // Empty nodes have no children.
  if (green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren greenChildren = green.getChildren();
  const auto &element = greenChildren.begin()->element;

  const size_t childOffset = getOffset();
  const size_t childIndex = 0;

  if (std::holds_alternative<GreenNode>(element)) {
    auto node =
        SyntaxNode(childOffset, childIndex, data, std::get<GreenNode>(element));
    return node;
  }

  if (std::holds_alternative<GreenToken>(element)) {
    auto token = SyntaxToken(childOffset, childIndex, data,
                             std::get<GreenToken>(element));
    return token;
  }

  return std::nullopt;
}

std::optional<SyntaxNode> SyntaxNode::getLastChild() const noexcept {
  const GreenNode &green = getGreen();

  // Empty nodes have no children.
  if (green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren greenChildren = green.getChildren();
  const auto end = greenChildren.rend();

  // Find the last node child, skipping any trailing tokens.
  size_t childIndex = green.getNumChildren() - 1;
  for (auto it = greenChildren.rbegin(); it != end; ++it, --childIndex) {
    const std::variant<GreenNode, GreenToken> element = it->element;
    if (std::holds_alternative<GreenNode>(element)) {
      const size_t childOffset = getOffset() + it->relativeOffset;
      return SyntaxNode(childOffset, childIndex, data,
                        std::get<GreenNode>(element));
    }
  }

  return std::nullopt;
}

std::optional<SyntaxElement> SyntaxNode::getLastChildOrToken() const noexcept {
  const GreenNode &green = getGreen();

  // Empty nodes have no children.
  if (green.getNumChildren() == 0) {
    return std::nullopt;
  }

  const GreenChildren greenChildren = green.getChildren();
  const GreenChildren::const_reverse_iterator it = greenChildren.rbegin();

  const size_t childOffset = getOffset() + it->relativeOffset;
  const size_t childIndex = green.getNumChildren() - 1;
  const std::variant<GreenNode, GreenToken> element = it->element;

  if (std::holds_alternative<GreenNode>(element)) {
    auto node =
        SyntaxNode(childOffset, childIndex, data, std::get<GreenNode>(element));
    return node;
  }

  if (std::holds_alternative<GreenToken>(element)) {
    auto token = SyntaxToken(childOffset, childIndex, data,
                             std::get<GreenToken>(element));
    return token;
  }

  util::yuzu_unreachable();
}

std::optional<SyntaxNode> SyntaxNode::getNextSibling() const noexcept {
  return data->getNextSibling();
}

std::optional<SyntaxElement>
SyntaxNode::getNextSiblingOrToken() const noexcept {
  return data->getNextSiblingOrToken();
}

std::optional<SyntaxNode> SyntaxNode::getPrevSibling() const noexcept {
  return data->getPrevSibling();
}

std::optional<SyntaxElement>
SyntaxNode::getPrevSiblingOrToken() const noexcept {
  return data->getPrevSiblingOrToken();
}

/// ===============
/// = SyntaxToken =
/// ===============
std::optional<SyntaxNode> SyntaxToken::getNextSibling() const noexcept {
  return data->getNextSibling();
}

std::optional<SyntaxElement>
SyntaxToken::getNextSiblingOrToken() const noexcept {
  return data->getNextSiblingOrToken();
}

std::optional<SyntaxNode> SyntaxToken::getPrevSibling() const noexcept {
  return data->getPrevSibling();
}

std::optional<SyntaxElement>
SyntaxToken::getPrevSiblingOrToken() const noexcept {
  return data->getPrevSiblingOrToken();
}
} // namespace yuzu::syntax
