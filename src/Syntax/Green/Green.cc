#include "Syntax/Green/Green.h"

#include "Util/ErrorHandling.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace yuzu::syntax {
GreenNode::GreenNode(SyntaxKind Kind, const std::vector<GreenElement> &Children)
    : Data_(std::make_shared<GreenNodeData>(
          GreenNodeData{.Kind = Kind,
                        .Width = computeWidth(Children),
                        .Children = std::move(Children)})) {}

size_t GreenNode::computeWidth(const std::vector<GreenElement> &Children) {
  size_t Width = 0;

  for (const GreenElement &Child : Children) {
    if (const std::optional<GreenNode> Node = Child.tryGetNode()) {
      Width += Node->getWidth();
      continue;
    }

    if (const std::optional<GreenToken> Token = Child.tryGetToken()) {
      Width += Token->getWidth();
      continue;
    }

    util::yuzu_unreachable();
  }

  return Width;
}

bool GreenNode::operator==(const GreenNode &Other) const noexcept {
  return Data_->Kind == Other.Data_->Kind &&
         Data_->Width == Other.Data_->Width &&
         Data_->Children == Other.Data_->Children;
}
} // namespace yuzu::syntax
