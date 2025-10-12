#include "Syntax/Green/Green.h"

#include "Util/ErrorHandling.h"

#include <cstdlib>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace yuzu::syntax {
/// =======================
/// = GreenNode::Iterator =
/// =======================
GreenNode::GreenNode::Iterator::reference
GreenNode::GreenNode::Iterator::operator*() const {
  return Node_->Data_->Children[Index_];
}

GreenNode::GreenNode::Iterator::pointer
GreenNode::GreenNode::Iterator::operator->() const {
  return &(Node_->Data_->Children[Index_]);
}

/// =============
/// = GreenNode =
/// =============
GreenNode::GreenNode(SyntaxKind Kind, GreenElement *Children,
                     size_t NumChildren, size_t Width) {

  const auto deleter = [Children, NumChildren](GreenNodeData *data) {
    if (Children) {
      for (size_t i = 0; i < NumChildren; ++i) {
        Children[i].~GreenElement();
      }
      std::free(Children);
    }
    delete data;
  };

  Data_ = std::shared_ptr<const GreenNodeData>(
      new GreenNodeData{.Children = Children,
                        .NumChildren = NumChildren,
                        .Width = Width,
                        .Kind = Kind},
      deleter);
}

GreenNode GreenNode::create(SyntaxKind Kind,
                            std::vector<GreenElement> Children) {
  const size_t NumChildren = Children.size();
  const size_t Width = computeWidth(Children);

  GreenElement *ChildrenArray = nullptr;

  if (NumChildren > 0) {
    ChildrenArray = static_cast<GreenElement *>(
        std::malloc(sizeof(GreenElement) * NumChildren));

    for (size_t i = 0; i < NumChildren; ++i) {
      new (&ChildrenArray[i]) GreenElement(std::move(Children[i]));
    }
  }

  return GreenNode(Kind, ChildrenArray, NumChildren, Width);
}

size_t GreenNode::computeWidth(const std::vector<GreenElement> &Children) {
  size_t Width = 0;

  for (const GreenElement &Child : Children) {
    if (const GreenNode *Node = Child.getIfNode()) {
      Width += Node->getWidth();
      continue;
    }

    if (const GreenToken *Token = Child.getIfToken()) {
      Width += Token->getWidth();
      continue;
    }

    util::yuzu_unreachable();
  }

  return Width;
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
