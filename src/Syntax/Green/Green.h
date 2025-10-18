#ifndef SYNTAX_GREEN_GREEN_H
#define SYNTAX_GREEN_GREEN_H

#include "Syntax/SyntaxKind.h"
#include "Util/ErrorHandling.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace yuzu::syntax {
class GreenChild;
class GreenChildren;
class GreenElement;
class GreenIterator;
class GreenNode;
class GreenToken;

struct GreenTokenData {
  /// The source code that this 'GreenToken' references. The source code is
  /// encoded directly in 'GreenTokenData' for use/reference outside of the
  /// source file it was defined in.
  const std::u32string Source;

  /// The kind of data this 'GreenToken' references.
  const SyntaxKind Kind;
};

struct GreenNodeData {
  /// A pointer to the start of the children of 'GreenNodeData', stored
  /// contiguously. Storing a raw pointer here is OK, and preferred over using
  /// standard containers like std::vector for a number of reasons.
  ///
  /// 1. 'GreenElements' (and 'GreenNodes', and 'GreenTokens') are immumtable.
  ///
  /// 2. Other standard containers, such as std::vector, either don't fit the
  /// use case exactly, or store extra memory. In the case of std::vector, and
  /// extra 8 bytes is used to track the capacity of the vector. Since
  /// 'GreenElement's are immutable, the size of the children will never change.
  /// We can take advantage of this fact by using a custom container that only
  /// uses 16 bytes (for the pointer, and the length).
  ///
  /// 3. Children drop with their parents, removing the risk of dangling
  /// pointers.
  const GreenChild *const Children;

  /// The number of children that this 'GreenNode' has.
  const size_t NumChildren;

  /// The relative size of this 'GreenNode' and it's children. To illustrate
  /// this, consider a 'GreenNode' with 3 'GreenToken's of that span 2
  /// characters. The 'Width' of the 'GreenNode' is 6, which is the sum of the
  /// widths of all of it's children.
  const size_t Width;

  /// The kind of data this 'GreenNode' references.
  const SyntaxKind Kind;
};

class GreenToken {
public:
  explicit GreenToken(const SyntaxKind Kind, const std::u32string &Source)
      : Data_(std::make_shared<const GreenTokenData>(
            GreenTokenData{.Source = std::move(Source), .Kind = Kind})) {}

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
  friend class GreenIterator;
  friend class GreenChildren;

  [[nodiscard]] static GreenNode create(SyntaxKind Kind,
                                        std::vector<GreenElement> Children);

  explicit GreenNode(SyntaxKind Kind, GreenChild *Children, size_t NumChildren,
                     size_t Width);

  GreenNode() = delete;

  /// Gets the kind of data this 'GreenNode' references.
  [[nodiscard]] SyntaxKind getKind() const noexcept { return Data_->Kind; }

  [[nodiscard]] size_t getWidth() const noexcept { return Data_->Width; }

  [[nodiscard]] GreenChildren getChildren() const noexcept;

  [[nodiscard]] size_t getNumChildren() const noexcept {
    return Data_->NumChildren;
  }

  [[nodiscard]] size_t getUseCount() const noexcept {
    return Data_.use_count();
  }

  bool operator==(const GreenNode &Other) const noexcept;

private:
  std::shared_ptr<const GreenNodeData> Data_;
};

class GreenElement {
public:
  explicit GreenElement(const GreenNode &Node) : Variant_(Node) {}
  explicit GreenElement(const GreenToken &Token) : Variant_(Token) {}
  explicit GreenElement(GreenNode &&Node) : Variant_(std::move(Node)) {}
  explicit GreenElement(GreenToken &&Token) : Variant_(std::move(Token)) {}

  GreenElement() = delete;

  [[nodiscard]] const GreenNode &getNode() const noexcept {
    return std::get<GreenNode>(Variant_);
  }

  [[nodiscard]] const GreenNode *getIfNode() const noexcept {
    return std::get_if<GreenNode>(&Variant_);
  }

  [[nodiscard]] const GreenToken &getToken() const noexcept {
    return std::get<GreenToken>(Variant_);
  }

  [[nodiscard]] const GreenToken *getIfToken() const noexcept {
    return std::get_if<GreenToken>(&Variant_);
  }

  [[nodiscard]] bool isNode() const noexcept {
    return std::holds_alternative<GreenNode>(Variant_);
  }

  [[nodiscard]] bool isToken() const noexcept {
    return std::holds_alternative<GreenToken>(Variant_);
  }

  [[nodiscard]] SyntaxKind getKind() const noexcept {
    if (const GreenNode *Node = getIfNode()) {
      return Node->getKind();
    }

    if (const GreenToken *Token = getIfToken()) {
      return Token->getKind();
    }

    util::yuzu_unreachable();
  }

  [[nodiscard]] size_t getWidth() const noexcept {
    if (const GreenNode *Node = getIfNode()) {
      return Node->getWidth();
    }

    if (const GreenToken *Token = getIfToken()) {
      return Token->getWidth();
    }

    util::yuzu_unreachable();
  }

  [[nodiscard]] size_t getUseCount() const noexcept {
    if (const GreenNode *Node = getIfNode()) {
      return Node->getUseCount();
    }

    if (const GreenToken *Token = getIfToken()) {
      return Token->getUseCount();
    }

    util::yuzu_unreachable();
  }

  bool operator==(const GreenElement &other) const {
    return Variant_ == other.Variant_;
  }

private:
  std::variant<GreenNode, GreenToken> Variant_;
};

class GreenChild {
public:
  explicit GreenChild(const size_t RelativeOffset, const GreenElement &Element)
      : RelativeOffset_(RelativeOffset), Element_(std::move(Element)) {}

  GreenChild() = delete;

  [[nodiscard]] size_t getRelativeOffset() const noexcept {
    return RelativeOffset_;
  }

  [[nodiscard]] const GreenElement &getElement() const noexcept {
    return Element_;
  }

  bool operator==(const GreenChild &other) const {
    return Element_ == other.Element_ &&
           RelativeOffset_ == other.RelativeOffset_;
  }

private:
  const size_t RelativeOffset_;
  const GreenElement Element_;
};
} // namespace yuzu::syntax

#endif // SYNTAX_GREEN_GREEN_H
