#ifndef SYNTAX_SYNTAX_ELEMENT_H_
#define SYNTAX_SYNTAX_ELEMENT_H_

#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "syntax/syntax_kind.h"
#include "syntax/syntax_node.h"
#include "syntax/syntax_token.h"

namespace yuzu::syntax {
class SyntaxElementData {
 public:
  explicit SyntaxElementData(const SyntaxNode& node) : variant_(node) {}
  explicit SyntaxElementData(const SyntaxToken& token) : variant_(token) {}
  SyntaxElementData() = delete;

  [[nodiscard]] SyntaxKind Kind() const noexcept {
    if (std::holds_alternative<SyntaxNode>(variant_)) {
      const SyntaxNode& node = std::get<SyntaxNode>(variant_);
      return node.Kind();
    }

    const SyntaxToken& token = std::get<SyntaxToken>(variant_);
    return token.Kind();
  }

  [[nodiscard]] std::optional<SyntaxNode> Parent() const noexcept {
    if (std::holds_alternative<SyntaxNode>(variant_)) {
      const SyntaxNode& node = std::get<SyntaxNode>(variant_);
      return node.Parent();
    } else {
      const SyntaxToken& token = std::get<SyntaxToken>(variant_);
      return token.Parent();
    }
  }

 private:
  const std::variant<SyntaxNode, SyntaxToken> variant_;
};

class SyntaxElement {
 public:
  explicit SyntaxElement(const SyntaxNode& node)
      : data_(std::make_shared<SyntaxElementData>(node)) {}

  explicit SyntaxElement(const SyntaxToken& token)
      : data_(std::make_shared<SyntaxElementData>(token)) {}

  SyntaxElement() = delete;

  [[nodiscard]] SyntaxKind Kind() const noexcept { return data_->Kind(); }

  [[nodiscard]] std::optional<SyntaxNode> Parent() const noexcept {
    return data_->Parent();
  }

 private:
  const std::shared_ptr<SyntaxElementData> data_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_SYNTAX_ELEMENT_H_
