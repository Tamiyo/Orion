#ifndef SYNTAX_SYNTAX_H
#define SYNTAX_SYNTAX_H

#include "Syntax/Green/Green.h"
#include "Syntax/SyntaxKind.h"
#include "Util/ErrorHandling.h"

#include <cstddef>
#include <optional>
#include <utility>
#include <variant>

namespace yuzu::syntax {
class SyntaxChildren;
class SyntaxChildrenWithTokens;
class SyntaxElement;
class SyntaxNode;
class SyntaxToken;

struct SyntaxData {
  [[nodiscard]] std::optional<const SyntaxNode> getNextSibling() const noexcept;

  [[nodiscard]] std::optional<const SyntaxElement>
  getNextSiblingOrToken() const noexcept;

  [[nodiscard]] std::optional<const SyntaxNode> getPrevSibling() const noexcept;

  [[nodiscard]] std::optional<const SyntaxElement>
  getPrevSiblingOrToken() const noexcept;

  /// The 'GreenElement' associated with this 'SyntaxData'. When parented to a
  /// 'SyntaxNode', this is a 'GreenNode'. When parented to a 'SyntaxToken',
  /// this is a 'GreenToken'.
  const GreenElement Green;

  /// The parent that this 'SyntaxNode' belongs to.
  const SyntaxNode *const Parent;

  /// The absolute offset of this 'SyntaxNode' in the source code.
  ///
  /// To illustrate this, in the code sample the character '(' has an absolute
  /// offset of 6.
  /// \code
  ///   print("hello world")
  /// \endcode
  const size_t Offset;

  /// The index of this 'SyntaxData' in the children of 'Parent'.
  const size_t Index;
};

class SyntaxNode {
public:
  static SyntaxNode createRoot(GreenNode Node) {
    return SyntaxNode(0, 0, nullptr, Node);
  }

  explicit SyntaxNode(size_t Offset, size_t Idx, const SyntaxNode *Parent,
                      GreenNode Green)
      : Data_(std::make_shared<SyntaxData>(
            SyntaxData{.Green = GreenElement(Green),
                       .Parent = Parent,
                       .Offset = Offset,
                       .Index = Idx})) {}

  SyntaxNode() = delete;

  [[nodiscard]] size_t getOffset() const noexcept { return Data_->Offset; }

  [[nodiscard]] const SyntaxNode *getParent() const noexcept {
    return Data_->Parent;
  }

  [[nodiscard]] const GreenNode &getGreen() const noexcept {
    return Data_->Green.getNode();
  }

  [[nodiscard]] SyntaxKind getKind() const noexcept {
    return Data_->Green.getKind();
  }

  [[nodiscard]] SyntaxChildren getChildren() const noexcept;

  [[nodiscard]] SyntaxChildrenWithTokens getChildrenWithTokens() const noexcept;

  [[nodiscard]] std::optional<const SyntaxNode> getFirstChild() const noexcept;

  [[nodiscard]] std::optional<const SyntaxElement>
  getFirstChildOrToken() const noexcept;

  [[nodiscard]] std::optional<const SyntaxNode> getLastChild() const noexcept;

  [[nodiscard]] std::optional<const SyntaxElement>
  getLastChildOrToken() const noexcept;

  [[nodiscard]] std::optional<const SyntaxNode> getNextSibling() const noexcept;

  [[nodiscard]] std::optional<const SyntaxElement>
  getNextSiblingOrToken() const noexcept;

  [[nodiscard]] std::optional<const SyntaxNode> getPrevSibling() const noexcept;

  [[nodiscard]] std::optional<const SyntaxElement>
  getPrevSiblingOrToken() const noexcept;

  bool operator==(const SyntaxNode &Other) const noexcept {
    return Data_->Offset == Other.Data_->Offset &&
           Data_->Parent == Other.Data_->Parent &&
           Data_->Green == Other.Data_->Green;
  }

private:
  std::shared_ptr<const SyntaxData> Data_;
};

class SyntaxToken {
public:
  explicit SyntaxToken(size_t Offset, size_t Idx, const SyntaxNode *Parent,
                       GreenToken Green)
      : Data_(std::make_shared<SyntaxData>(
            SyntaxData{.Green = GreenElement(Green),
                       .Parent = Parent,
                       .Offset = Offset,
                       .Index = Idx})) {}

  explicit SyntaxToken(size_t Offset, size_t Idx, GreenToken Green)
      : Data_(std::make_shared<SyntaxData>(
            SyntaxData{.Green = GreenElement(Green),
                       .Parent = nullptr,
                       .Offset = Offset,
                       .Index = Idx})) {}

  SyntaxToken() = delete;

  [[nodiscard]] size_t getOffset() const noexcept { return Data_->Offset; }

  [[nodiscard]] const SyntaxNode *getParent() const noexcept {
    return Data_->Parent;
  }

  [[nodiscard]] const GreenToken &getGreen() const noexcept {
    return Data_->Green.getToken();
  }

  [[nodiscard]] SyntaxKind getKind() const noexcept {
    return Data_->Green.getKind();
  }

  [[nodiscard]] std::optional<const SyntaxNode> getNextSibling() const noexcept;

  [[nodiscard]] std::optional<const SyntaxElement>
  getNextSiblingOrToken() const noexcept;

  [[nodiscard]] std::optional<const SyntaxNode> getPrevSibling() const noexcept;

  [[nodiscard]] std::optional<const SyntaxElement>
  getPrevSiblingOrToken() const noexcept;

  bool operator==(const SyntaxToken &Other) const noexcept {
    return Data_->Offset == Other.Data_->Offset &&
           Data_->Parent == Other.Data_->Parent &&
           Data_->Green == Other.Data_->Green;
  }

private:
  std::shared_ptr<const SyntaxData> Data_;
};

class SyntaxElement {
public:
  explicit SyntaxElement(SyntaxNode &Node) : Variant_(std::move(Node)) {}

  explicit SyntaxElement(SyntaxToken &Token) : Variant_(std::move(Token)) {}

  SyntaxElement() = delete;

  [[nodiscard]] const SyntaxNode &getNode() const noexcept {
    return std::get<SyntaxNode>(Variant_);
  }

  [[nodiscard]] const SyntaxNode *getIfNode() const noexcept {
    return std::get_if<SyntaxNode>(&Variant_);
  }

  [[nodiscard]] const SyntaxToken &getToken() const noexcept {
    return std::get<SyntaxToken>(Variant_);
  }

  [[nodiscard]] const SyntaxToken *getIfToken() const noexcept {
    return std::get_if<SyntaxToken>(&Variant_);
  }

  [[nodiscard]] bool isNode() const noexcept {
    return std::holds_alternative<SyntaxNode>(Variant_);
  }

  [[nodiscard]] bool isToken() const noexcept {
    return std::holds_alternative<SyntaxToken>(Variant_);
  }

  [[nodiscard]] std::optional<const SyntaxNode>
  getNextSibling() const noexcept {
    if (const SyntaxNode *Node = getIfNode()) {
      return Node->getNextSibling();
    }

    if (const SyntaxToken *Token = getIfToken()) {
      return Token->getNextSibling();
    }

    util::yuzu_unreachable();
  }

  [[nodiscard]] std::optional<const SyntaxElement>
  getNextSiblingOrToken() const noexcept {
    if (const SyntaxNode *Node = getIfNode()) {
      return Node->getNextSiblingOrToken();
    }

    if (const SyntaxToken *Token = getIfToken()) {
      return Token->getNextSiblingOrToken();
    }

    util::yuzu_unreachable();
  }

  [[nodiscard]] std::optional<const SyntaxNode>
  getPrevSibling() const noexcept {
    if (const SyntaxNode *Node = getIfNode()) {
      return Node->getPrevSibling();
    }

    if (const SyntaxToken *Token = getIfToken()) {
      return Token->getPrevSibling();
    }

    util::yuzu_unreachable();
  }

  [[nodiscard]] std::optional<const SyntaxElement>
  getPrevSiblingOrToken() const noexcept {
    if (const SyntaxNode *Node = getIfNode()) {
      return Node->getPrevSiblingOrToken();
    }

    if (const SyntaxToken *Token = getIfToken()) {
      return Token->getPrevSiblingOrToken();
    }

    util::yuzu_unreachable();
  }

  bool operator==(const SyntaxElement &other) const {
    return Variant_ == other.Variant_;
  }

private:
  std::variant<SyntaxNode, SyntaxToken> Variant_;
};
} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_H
