#ifndef SYNTAX_GREEN_GREEN_ELEMENT_H_
#define SYNTAX_GREEN_GREEN_ELEMENT_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>

#include "syntax/green/green_node.h"
#include "syntax/green/green_token.h"
#include "syntax/syntax_kind.h"

namespace yuzu::syntax {
class GreenElementData {
 public:
  explicit GreenElementData(const GreenNode& node) : variant_(node) {}
  explicit GreenElementData(const GreenToken& token) : variant_(token) {}

  GreenElementData() = delete;

  [[nodiscard]] std::optional<GreenNode> TryGetNode() const noexcept {
    if (auto p = std::get_if<GreenNode>(&variant_)) {
      return *p;
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<GreenToken> TryGetToken() const noexcept {
    if (auto p = std::get_if<GreenToken>(&variant_)) {
      return *p;
    }
    return std::nullopt;
  }

  [[nodiscard]] bool IsNode() const noexcept {
    return std::holds_alternative<GreenNode>(variant_);
  }

  [[nodiscard]] bool IsToken() const noexcept {
    return std::holds_alternative<GreenToken>(variant_);
  }

  bool operator==(const GreenElementData& other) const noexcept {
    return variant_ == other.variant_;
  }

 private:
  std::variant<GreenNode, GreenToken> variant_;
};

class GreenElement {
 public:
  explicit GreenElement(const GreenNode& node)
      : data_(std::make_shared<GreenElementData>(node)) {}

  explicit GreenElement(const GreenToken& token)
      : data_(std::make_shared<GreenElementData>(token)) {}

  [[nodiscard]] std::optional<GreenNode> TryGetNode() const noexcept {
    return data_->TryGetNode();
  }

  [[nodiscard]] std::optional<GreenToken> TryGetToken() const noexcept {
    return data_->TryGetToken();
  }

  [[nodiscard]] size_t UseCount() const noexcept { return data_.use_count(); }

  [[nodiscard]] bool IsNode() const noexcept { return data_->IsNode(); }

  [[nodiscard]] bool IsToken() const noexcept { return data_->IsToken(); }

  bool operator==(const GreenElement& other) const noexcept {
    return *data_ == *other.data_;
  }

 private:
  std::shared_ptr<GreenElementData> data_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_GREEN_GREEN_ELEMENT_H_
