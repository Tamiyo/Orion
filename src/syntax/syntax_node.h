#ifndef SYNTAX_SYNTAX_NODE_H_
#define SYNTAX_SYNTAX_NODE_H_

#include <memory>
#include <optional>
#include <utility>

#include "syntax/green/green_node.h"

namespace yuzu::syntax {
class SyntaxNode;

class SyntaxNodeData {
 public:
  explicit SyntaxNodeData(const size_t offset, std::optional<SyntaxNode> parent,
                          GreenNode green)
      : offset_(offset), parent_(std::move(parent)), green_(std::move(green)) {}

  SyntaxNodeData() = delete;

  [[nodiscard]] size_t Offset() const noexcept { return offset_; }

  [[nodiscard]] const std::optional<SyntaxNode>& Parent() const noexcept {
    return parent_;
  }

  [[nodiscard]] const GreenNode& Green() const noexcept { return green_; }

 private:
  const size_t offset_;
  const std::optional<SyntaxNode> parent_;
  const GreenNode green_;
};

class SyntaxNode {
 public:
  static SyntaxNode CreateRoot(const GreenNode& node) {
    return SyntaxNode(0, node);
  }

  explicit SyntaxNode(size_t offset, SyntaxNode parent, GreenNode green)
      : data_(std::make_shared<SyntaxNodeData>(
            offset, std::make_optional(std::move(parent)), std::move(green))) {}

  explicit SyntaxNode(size_t offset, GreenNode green)
      : data_(std::make_shared<SyntaxNodeData>(offset, std::nullopt,
                                               std::move(green))) {}

  SyntaxNode() = delete;

  [[nodiscard]] size_t Offset() const noexcept { return data_->Offset(); }

  [[nodiscard]] const std::optional<SyntaxNode>& Parent() const noexcept {
    return data_->Parent();
  }

  [[nodiscard]] const GreenNode& Green() const noexcept {
    return data_->Green();
  }

  [[nodiscard]] SyntaxKind Kind() const noexcept {
    return data_->Green().Kind();
  }

 private:
  const std::shared_ptr<SyntaxNodeData> data_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_SYNTAX_NODE_H_
