#ifndef SYNTAX_SYNTAX_H
#define SYNTAX_SYNTAX_H

#include <memory>
#include <optional>
#include <utility>
#include <variant>

#include "Syntax/Green/Green.h"
#include "Syntax/SyntaxKind.h"

namespace yuzu::syntax {
class SyntaxNode;

struct SyntaxData {
  const size_t Offset;
  const SyntaxNode *Parent;
  const GreenElement Green;
};

class SyntaxNode : public std::enable_shared_from_this<SyntaxNode> {
public:
  static SyntaxNode createRoot(GreenNode Node) {
    return SyntaxNode(0, nullptr, Node);
  }

  explicit SyntaxNode(size_t Offset, const SyntaxNode *Parent, GreenNode Green)
      : Data_(std::make_shared<SyntaxData>(
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

  bool operator==(const SyntaxNode &Other) const noexcept {
    return Data_->Offset == Other.Data_->Offset &&
           Data_->Parent == Other.Data_->Parent &&
           Data_->Green == Other.Data_->Green;
  }

private:
  std::shared_ptr<SyntaxData> Data_;
};

class SyntaxToken {
public:
  explicit SyntaxToken(size_t Offset, SyntaxNode *Parent, GreenNode Green)
      : Data_(std::make_shared<SyntaxData>(
            SyntaxData{.Offset = Offset,
                       .Parent = Parent,
                       .Green = GreenElement(Green)})) {}

  explicit SyntaxToken(size_t Offset, GreenNode Green)
      : Data_(std::make_shared<SyntaxData>(
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
  std::shared_ptr<SyntaxData> Data_;
};
} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_H
