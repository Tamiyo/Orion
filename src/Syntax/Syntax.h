#ifndef SYNTAX_SYNTAX_H
#define SYNTAX_SYNTAX_H

#include "Syntax/Green/Green.h"
#include "Syntax/SyntaxKind.h"

#include <memory>
#include <optional>
#include <utility>
#include <variant>

namespace yuzu::syntax {
class SyntaxNode;
class SyntaxChildren;
class SyntaxChildrenWithTokens;

struct SyntaxData {
  const size_t Offset;
  const SyntaxNode *Parent;
  const GreenElement Green;
};

class SyntaxNode {
public:
  static SyntaxNode createRoot(GreenNode Node) {
    return SyntaxNode(0, nullptr, Node);
  }

  explicit SyntaxNode(size_t Offset, const SyntaxNode *Parent, GreenNode Green)
      : Data_(std::make_unique<SyntaxData>(
            SyntaxData{.Offset = Offset,
                       .Parent = Parent,
                       .Green = GreenElement(Green)})) {}

  SyntaxNode() = delete;

  [[nodiscard]] size_t getOffset() const noexcept { return Data_->Offset; }

  [[nodiscard]] const SyntaxNode *getParent() const noexcept {
    return Data_->Parent;
  }

  [[nodiscard]] GreenNode getGreen() const noexcept {
    return Data_->Green.tryGetNode().value();
  }

  [[nodiscard]] SyntaxKind getKind() const noexcept {
    return Data_->Green.getKind();
  }

  [[nodiscard]] SyntaxChildren getChildren() const;

  [[nodiscard]] SyntaxChildrenWithTokens getChildrenWithTokens() const;

  bool operator==(const SyntaxNode &Other) const noexcept {
    return Data_->Offset == Other.Data_->Offset &&
           Data_->Parent == Other.Data_->Parent &&
           Data_->Green == Other.Data_->Green;
  }

private:
  std::unique_ptr<SyntaxData> Data_;
};

class SyntaxToken {
public:
  explicit SyntaxToken(size_t Offset, const SyntaxNode *Parent,
                       GreenToken Green)
      : Data_(std::make_unique<SyntaxData>(
            SyntaxData{.Offset = Offset,
                       .Parent = Parent,
                       .Green = GreenElement(Green)})) {}

  explicit SyntaxToken(size_t Offset, GreenToken Green)
      : Data_(std::make_unique<SyntaxData>(
            SyntaxData{.Offset = Offset,
                       .Parent = nullptr,
                       .Green = GreenElement(Green)})) {}

  SyntaxToken() = delete;

  [[nodiscard]] size_t getOffset() const noexcept { return Data_->Offset; }

  [[nodiscard]] const SyntaxNode *getParent() const noexcept {
    return Data_->Parent;
  }

  [[nodiscard]] GreenToken getGreen() const noexcept {
    return Data_->Green.tryGetToken().value();
  }

  [[nodiscard]] SyntaxKind getKind() const noexcept {
    return Data_->Green.getKind();
  }

  bool operator==(const SyntaxToken &Other) const noexcept {
    return Data_->Offset == Other.Data_->Offset &&
           Data_->Parent == Other.Data_->Parent &&
           Data_->Green == Other.Data_->Green;
  }

private:
  std::unique_ptr<SyntaxData> Data_;
};

using SyntaxElement = std::variant<SyntaxNode, SyntaxToken>;
} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_H
