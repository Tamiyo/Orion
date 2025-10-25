#ifndef YUZU_SYNTAX_SYNTAX_H
#define YUZU_SYNTAX_SYNTAX_H

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/SyntaxKind.h"
#include "yuzu/Util/ErrorHandling.h"

#include <cstddef>
#include <optional>
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

class SyntaxNode final {
public:
  static SyntaxNode createRoot(GreenNode Node) {
    return SyntaxNode(0, 0, nullptr, Node);
  }

  explicit SyntaxNode(size_t Offset, size_t Idx, const SyntaxNode *Parent,
                      GreenNode Green)
      : Data_(std::make_shared<SyntaxData>(SyntaxData{.Green = Green,
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

class SyntaxToken final {
public:
  explicit SyntaxToken(size_t Offset, size_t Idx, const SyntaxNode *Parent,
                       GreenToken Green)
      : Data_(std::make_shared<SyntaxData>(SyntaxData{.Green = Green,
                                                      .Parent = Parent,
                                                      .Offset = Offset,
                                                      .Index = Idx})) {}

  explicit SyntaxToken(size_t Offset, size_t Idx, GreenToken Green)
      : Data_(std::make_shared<SyntaxData>(SyntaxData{.Green = Green,
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

class SyntaxElement final : public std::variant<SyntaxNode, SyntaxToken> {
public:
  using std::variant<SyntaxNode, SyntaxToken>::variant;

  SyntaxElement() = delete;

  [[nodiscard]] const SyntaxNode &getNode() const noexcept {
    return std::get<SyntaxNode>(*this);
  }

  [[nodiscard]] const SyntaxNode *getIfNode() const noexcept {
    return std::get_if<SyntaxNode>(this);
  }

  [[nodiscard]] const SyntaxToken &getToken() const noexcept {
    return std::get<SyntaxToken>(*this);
  }

  [[nodiscard]] const SyntaxToken *getIfToken() const noexcept {
    return std::get_if<SyntaxToken>(this);
  }

  [[nodiscard]] bool isNode() const noexcept {
    return std::holds_alternative<SyntaxNode>(*this);
  }

  [[nodiscard]] bool isToken() const noexcept {
    return std::holds_alternative<SyntaxToken>(*this);
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
};
} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_SYNTAX_H
