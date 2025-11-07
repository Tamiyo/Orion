#include "yuzu/Syntax/Green/Green.h"

#include "yuzu/Syntax/Green/GreenIterator.h"

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace yuzu::syntax {
/// =============
/// = GreenNode =
/// =============
GreenNode::GreenNode(SyntaxKind Kind, GreenChild *Children, size_t NumChildren,
                     size_t Width) {

  const auto deleter = [Children, NumChildren](GreenNodeData *data) {
    if (Children) {
      for (size_t i = 0; i < NumChildren; ++i) {
        Children[i].~GreenChild();
      }
      std::free(Children);
    }
    delete data;
  };

  Data_ = std::shared_ptr<const GreenNodeData>(
      new GreenNodeData{.NumChildren = NumChildren,
                        .Kind = Kind,
                        .Width = Width,
                        .Children = Children},
      deleter);
}

GreenNode GreenNode::create(SyntaxKind Kind,
                            std::vector<GreenElement> Children) {
  const size_t NumChildren = Children.size();
  // const size_t Width = computeWidth(Children);

  GreenChild *ChildrenArray = nullptr;
  size_t Width = 0;

  if (NumChildren > 0) {
    ChildrenArray = static_cast<GreenChild *>(
        std::malloc(sizeof(GreenChild) * NumChildren));

    for (size_t i = 0; i < NumChildren; ++i) {
      const size_t RelativeOffset = Width;
      Width += Children[i].getWidth();

      new (&ChildrenArray[i]) GreenChild{.Element = std::move(Children[i]),
                                         .RelativeOffset = RelativeOffset};
    }
  }

  return GreenNode(Kind, ChildrenArray, NumChildren, Width);
}

GreenChildren GreenNode::getChildren() const noexcept {
  return GreenChildren(this);
}

bool GreenNode::operator==(const GreenNode &Other) const noexcept {
  if (Data_->Kind != Other.Data_->Kind || Data_->Width != Other.Data_->Width ||
      Data_->NumChildren != Other.Data_->NumChildren) {
    return false;
  }

  for (size_t i = 0; i < Data_->NumChildren; ++i) {
    if (!(Data_->Children[i] == Other.Data_->Children[i])) {
      return false;
    }
  }

  return true;
}
} // namespace yuzu::syntax
