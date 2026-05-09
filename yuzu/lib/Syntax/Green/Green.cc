#include "yuzu/Syntax/Green/Green.h"

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <utility>
#include <vector>

namespace yuzu::syntax {
/// =============
/// = GreenNode =
/// =============
GreenNode::GreenNode(SyntaxKind kind, GreenChild *children, size_t numChildren,
                     size_t width) {

  const auto deleter = [children, numChildren](GreenNodeData *data) {
    if (children) {
      for (size_t i = 0; i < numChildren; ++i) {
        children[i].~GreenChild();
      }
      std::free(children);
    }
    delete data;
  };

  data = std::shared_ptr<const GreenNodeData>(
      new GreenNodeData{.numChildren = numChildren,
                        .width = width,
                        .kind = kind,
                        .children = children},
      deleter);
}

GreenNode GreenNode::create(SyntaxKind kind,
                            std::vector<GreenElement> children) {
  const size_t numChildren = children.size();

  GreenChild *childrenArray = nullptr;
  size_t width = 0;

  if (numChildren > 0) {
    childrenArray = static_cast<GreenChild *>(
        std::malloc(sizeof(GreenChild) * numChildren));

    for (size_t i = 0; i < numChildren; ++i) {
      const size_t relativeOffset = width;
      width += children[i].getWidth();

      new (&childrenArray[i]) GreenChild{.element = std::move(children[i]),
                                         .relativeOffset = relativeOffset};
    }
  }

  return GreenNode(kind, childrenArray, numChildren, width);
}

GreenChildren GreenNode::getChildren() const { return GreenChildren(this); }

bool GreenNode::operator==(const GreenNode &other) const {
  if (data->kind != other.data->kind || data->width != other.data->width ||
      data->numChildren != other.data->numChildren) {
    return false;
  }

  for (size_t i = 0; i < data->numChildren; ++i) {
    if (!(data->children[i] == other.data->children[i])) {
      return false;
    }
  }

  return true;
}
} // namespace yuzu::syntax
