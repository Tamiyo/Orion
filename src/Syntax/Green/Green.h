#ifndef SYNTAX_GREEN_GREEN_H
#define SYNTAX_GREEN_GREEN_H

#include "Syntax/SyntaxKind.h"

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace yuzu::syntax {

class GreenElement;

struct GreenTokenData {
  const SyntaxKind Kind;
  const std::u32string Source;
};

struct GreenNodeData {
  const SyntaxKind Kind;
  const size_t Width;
  const std::vector<GreenElement> Children;
};

class GreenToken {
public:
  explicit GreenToken(const SyntaxKind Kind, const std::u32string &Source)
      : Data_(std::make_shared<const GreenTokenData>(
            GreenTokenData{Kind, std::move(Source)})) {}

  GreenToken() = delete;

  [[nodiscard]] SyntaxKind getKind() const noexcept { return Data_->Kind; }

  [[nodiscard]] std::u32string_view getSource() const noexcept {
    return Data_->Source;
  }

  [[nodiscard]] size_t getWidth() const noexcept {
    return Data_->Source.size();
  }

  [[nodiscard]] size_t getUseCount() const noexcept {
    return Data_.use_count();
  }

  bool operator==(const GreenToken &Other) const noexcept {
    return Data_->Kind == Other.Data_->Kind &&
           Data_->Source == Other.Data_->Source;
  }

private:
  std::shared_ptr<const GreenTokenData> Data_;
};

class GreenNode {
public:
  explicit GreenNode(SyntaxKind Kind,
                     const std::vector<GreenElement> &Children);
  GreenNode() = delete;

  [[nodiscard]] SyntaxKind getKind() const noexcept { return Data_->Kind; }

  [[nodiscard]] size_t getWidth() const noexcept { return Data_->Width; }

  [[nodiscard]] const std::vector<GreenElement> &getChildren() const noexcept {
    return Data_->Children;
  }

  [[nodiscard]] size_t getUseCount() const noexcept {
    return Data_.use_count();
  }

  bool operator==(const GreenNode &Other) const noexcept;

  static size_t computeWidth(const std::vector<GreenElement> &Children);

private:
  std::shared_ptr<const GreenNodeData> Data_;
};

class GreenElement {
public:
  explicit GreenElement(GreenNode Node) : Variant_(Node) {}
  explicit GreenElement(GreenToken Token) : Variant_(Token) {}

  GreenElement() = delete;

  [[nodiscard]] std::optional<GreenNode> tryGetNode() const noexcept {
    if (auto P = std::get_if<GreenNode>(&Variant_)) {
      return *P;
    }
    return std::nullopt;
  }

  [[nodiscard]] std::optional<GreenToken> tryGetToken() const noexcept {
    if (auto P = std::get_if<GreenToken>(&Variant_)) {
      return *P;
    }
    return std::nullopt;
  }

  [[nodiscard]] bool isNode() const noexcept {
    return std::holds_alternative<GreenNode>(Variant_);
  }

  [[nodiscard]] bool isToken() const noexcept {
    return std::holds_alternative<GreenToken>(Variant_);
  }

  [[nodiscard]] SyntaxKind getKind() const noexcept {
    if (std::holds_alternative<GreenNode>(Variant_)) {
      return std::get<GreenNode>(Variant_).getKind();
    }

    return std::get<GreenToken>(Variant_).getKind();
  }

  [[nodiscard]] size_t getWidth() const noexcept {
    if (std::holds_alternative<GreenNode>(Variant_)) {
      return std::get<GreenNode>(Variant_).getWidth();
    }

    return std::get<GreenToken>(Variant_).getSource().size();
  }

  [[nodiscard]] long getUseCount() const noexcept {
    if (const GreenNode *P = std::get_if<GreenNode>(&Variant_)) {
      return P->getUseCount();
    }

    if (const GreenToken *P = std::get_if<GreenToken>(&Variant_)) {
      return P->getUseCount();
    }

    return 0;
  }

  bool operator==(const GreenElement &Other) const noexcept {
    return Variant_ == Other.Variant_;
  }

private:
  std::variant<GreenNode, GreenToken> Variant_;
};
} // namespace yuzu::syntax

#endif // SYNTAX_GREEN_GREEN_H
