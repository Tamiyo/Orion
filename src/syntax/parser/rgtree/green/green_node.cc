#include "syntax/parser/rgtree/green/green_node.h"

#include "syntax/parser/rgtree/green/green_element.h"

namespace orion::syntax {
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