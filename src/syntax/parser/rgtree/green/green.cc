#include "syntax/parser/rgtree/green/green.h"

#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace orion::syntax {
size_t GreenElement::UseCount() const noexcept {
  if (std::holds_alternative<GreenNode>(variant_)) {
    return std::get<GreenNode>(variant_).UseCount();
  }

  if (std::holds_alternative<GreenToken>(variant_)) {
    return std::get<GreenToken>(variant_).UseCount();
  }

  return 0;  // No shared data for monostate.
}

GreenNodeData::GreenNodeData(const SyntaxKind kind, const size_t width,
                             std::vector<GreenElement> children)
    : kind_(kind), width_(width), children_(std::move(children)) {}

bool orion::syntax::GreenNodeData::operator==(
    const GreenNodeData& other) const {
  return kind_ == other.kind_ && width_ == other.width_ &&
         children_ == other.children_;
}

GreenNode::GreenNode(const SyntaxKind kind,
                     const std::vector<GreenElement>& children)
    : data_(std::make_shared<GreenNodeData>(
          GreenNodeData(kind, ComputeWidth(children), children))) {}

size_t GreenNode::ComputeWidth(const std::vector<GreenElement>& children) {
  size_t width = 0;

  for (const GreenElement& child : children) {
    if (const std::optional<GreenNode> node = child.TryGetNode();
        node.has_value()) {
      width += node.value().Width();
      continue;
    }

    if (const std::optional<GreenToken> token = child.TryGetToken();
        token.has_value()) {
      width += token.value().Source().size();
      continue;
    }

    throw std::invalid_argument("unknown with object");
  }

  return width;
}

}  // namespace orion::syntax
