#ifndef SYNTAX_GREEN_GREEN_NODE_H_
#define SYNTAX_GREEN_GREEN_NODE_H_

#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include "syntax/green/green_token.h"
#include "syntax/syntax_kind.h"

namespace yuzu::syntax {
class GreenElement;

class GreenNodeData {
 public:
  explicit GreenNodeData(const SyntaxKind kind,
                         const std::vector<GreenElement>& children);

  GreenNodeData() = delete;

  [[nodiscard]] SyntaxKind Kind() const noexcept { return kind_; }

  [[nodiscard]] size_t Width() const noexcept { return width_; }

  [[nodiscard]] const std::vector<GreenElement>& Children() const noexcept {
    return children_;
  }

  [[nodiscard]] bool operator==(const GreenNodeData& other) const noexcept;

 private:
  static size_t ComputeWidth(const std::vector<GreenElement>& children);

  const SyntaxKind kind_;
  const size_t width_;
  const std::vector<GreenElement> children_;
};

class GreenNode {
 public:
  explicit GreenNode(SyntaxKind kind,
                     const std::vector<GreenElement>& children);

  GreenNode() = delete;
  GreenNode(const GreenNode&) = default;
  GreenNode(GreenNode&&) = default;

  [[nodiscard]] SyntaxKind Kind() const noexcept { return data_->Kind(); }

  [[nodiscard]] size_t Width() const noexcept { return data_->Width(); }

  [[nodiscard]] size_t UseCount() const noexcept { return data_.use_count(); }

  [[nodiscard]] const std::vector<GreenElement>& Children() const noexcept {
    return data_->Children();
  }

  bool operator==(const GreenNode& other) const noexcept {
    return data_ == other.data_;
  }

 private:
  const std::shared_ptr<GreenNodeData> data_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_GREEN_GREEN_NODE_H_
