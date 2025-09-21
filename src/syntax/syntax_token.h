#ifndef SYNTAX_SYNTAX_TOKEN_H_
#define SYNTAX_SYNTAX_TOKEN_H_

#include <memory>
#include <optional>
#include <utility>

#include "syntax/green/green_token.h"
#include "syntax/syntax_node.h"

namespace yuzu::syntax {
class SyntaxTokenData {
 public:
  explicit SyntaxTokenData(const size_t offset,
                           std::optional<SyntaxNode> parent, GreenToken green)
      : offset_(offset), parent_(std::move(parent)), green_(std::move(green)) {}

  SyntaxTokenData() = delete;

  [[nodiscard]] size_t Offset() const { return offset_; }

  [[nodiscard]] const std::optional<SyntaxNode>& Parent() const {
    return parent_;
  }

  [[nodiscard]] const GreenToken& Green() const { return green_; }

 private:
  const size_t offset_;
  const std::optional<SyntaxNode> parent_;
  const GreenToken green_;
};

class SyntaxToken {
 public:
  explicit SyntaxToken(size_t offset, const SyntaxNode& parent,
                       const GreenToken& green)
      : data_(std::make_shared<SyntaxTokenData>(
            offset, std::make_optional(parent), green)) {}

  explicit SyntaxToken(size_t offset, const GreenToken& green)
      : data_(std::make_shared<SyntaxTokenData>(offset, std::nullopt, green)) {}

  SyntaxToken() = delete;

  [[nodiscard]] size_t Offset() const noexcept { return data_->Offset(); }

  [[nodiscard]] const std::optional<SyntaxNode>& Parent() const noexcept {
    return data_->Parent();
  }

  [[nodiscard]] const GreenToken& Green() const noexcept {
    return data_->Green();
  }

  [[nodiscard]] SyntaxKind Kind() const noexcept {
    return data_->Green().Kind();
  }

 private:
  const std::shared_ptr<SyntaxTokenData> data_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_SYNTAX_TOKEN_H_
