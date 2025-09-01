#include "syntax/green/green_node.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "syntax/green/green_element.h"
#include "syntax/green/green_token.h"
#include "syntax/syntax_kind.h"

namespace yuzu::syntax {
GreenNodeData::GreenNodeData(const SyntaxKind kind,
                             const std::vector<GreenElement>& children)
    : kind_(kind), width_(), children_(std::move(children)) {}

bool GreenNodeData::operator==(const GreenNodeData& other) const noexcept {
  return kind_ == other.kind_ && width_ == other.width_ &&
         children_ == other.children_;
}

size_t GreenNodeData::ComputeWidth(const std::vector<GreenElement>& children) {
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

GreenNode::GreenNode(SyntaxKind kind, const std::vector<GreenElement>& children)
    : data_(std::make_shared<GreenNodeData>(GreenNodeData(kind, children))) {}
};  // namespace yuzu::syntax
