#ifndef YUZU_SYNTAX_GREEN_GREEN_H
#define YUZU_SYNTAX_GREEN_GREEN_H

#include "yuzu/Syntax/SyntaxKind.h"
#include "yuzu/Util/ErrorHandling.h"

#include <cstddef>
#include <memory>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace yuzu::syntax {
struct GreenChild;
class GreenChildren;
class GreenElement;
class GreenIterator;
class GreenNode;
class GreenToken;

/// \brief Data structure for immutable green tokens.
///
/// GreenTokenData stores the actual content and metadata for a token in the
/// green tree. Green tokens are immutable and represent terminal nodes
/// (leaves) in the syntax tree.
struct [[nodiscard]] GreenTokenData {
  /// The source code that this 'GreenToken' references. The source code is
  /// encoded directly in 'GreenTokenData' for use/reference outside of the
  /// source file it was defined in.
  const std::u32string_view source;

  /// The kind of data this 'GreenToken' references.
  const SyntaxKind kind;
};

/// \brief Data structure for immutable green nodes.
///
/// GreenNodeData stores the metadata for a non-terminal node in the green
/// tree. Green nodes are immutable and can contain both child nodes and
/// tokens.
struct [[nodiscard]] GreenNodeData {
  /// The number of children that this 'GreenNode' has.
  const size_t numChildren;

  /// The kind of data this 'GreenNode' references.
  const SyntaxKind kind;

  /// The relative size of this 'GreenNode' and its children. To illustrate
  /// this, consider a 'GreenNode' with 3 'GreenToken's that span 2
  /// characters. The 'Width' of the 'GreenNode' is 6, which is the sum of the
  /// widths of all of its children.
  const size_t width;

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
  const GreenChild *const children;
};

/// \brief An immutable token in the green tree.
///
/// GreenToken represents a terminal element (leaf) in the syntax tree,
/// such as keywords, identifiers, literals, and operators. Green tokens
/// are immutable and use shared ownership through std::shared_ptr for
/// efficient memory management and sharing across the tree.
class [[nodiscard]] GreenToken final {
public:
  /// \brief Construct a GreenToken.
  ///
  /// \param kind The syntax kind of this token.
  /// \param source The source text content of this token.
  explicit GreenToken(const SyntaxKind kind, const std::u32string_view &source)
      : data(std::make_shared<const GreenTokenData>(
            GreenTokenData{.source = std::move(source), .kind = kind})) {}

  /// Deleted default constructor to enforce proper initialization.
  GreenToken() = delete;

  /// \brief Get the syntax kind of this token.
  ///
  /// \return The SyntaxKind of this token.
  [[nodiscard]] SyntaxKind getKind() const noexcept { return data->kind; }

  /// \brief Get the source text of this token.
  ///
  /// \return A view of the source text content.
  [[nodiscard]] std::u32string_view getSource() const noexcept {
    return data->source;
  }

  /// \brief Get the width of this token.
  ///
  /// \return The number of characters in the source text.
  [[nodiscard]] size_t getWidth() const noexcept { return data->source.size(); }

  /// \brief Get the reference count for this token's data.
  ///
  /// \return The number of references to the underlying data.
  [[nodiscard]] size_t getUseCount() const noexcept { return data.use_count(); }

  /// \brief Equality comparison operator.
  ///
  /// \param other The GreenToken to compare with.
  /// \return True if both tokens have the same kind and source text.
  bool operator==(const GreenToken &other) const noexcept {
    return data->kind == other.data->kind && data->source == other.data->source;
  }

  /// \brief Inequality comparison operator.
  ///
  /// \param other The GreenToken to compare with.
  /// \return True if the tokens differ in kind or source text.
  bool operator!=(const GreenToken &other) const noexcept {
    return data->kind != other.data->kind || data->source != other.data->source;
  }

private:
  std::shared_ptr<const GreenTokenData> data;
};

/// \brief An immutable node in the green tree.
///
/// GreenNode represents a non-terminal element in the syntax tree that
/// contains child nodes and/or tokens. Green nodes are immutable and use
/// shared ownership through std::shared_ptr for efficient memory management
/// and structural sharing across the tree.
class [[nodiscard]] GreenNode final {
public:
  friend class GreenIterator;
  friend class GreenChildren;

  /// \brief Create a GreenNode from a vector of children.
  ///
  /// This method is the preferred way to construct GreenNodes, instead of the
  /// default constructor. This method performs additional work computing
  /// "GreenChild"ren, such as pre-computing the size of each GreenChild.
  ///
  /// \param kind The syntax kind of this node.
  /// \param children The child elements (nodes and/or tokens) of this node.
  /// \return A new GreenNode containing the specified children.
  static GreenNode create(SyntaxKind kind, std::vector<GreenElement> children);

  /// \brief Construct a GreenNode.
  ///
  /// \param kind The syntax kind of this node.
  /// \param children Pointer to the array of child elements.
  /// \param numChildren The number of children in the array.
  /// \param width The total width of this node and all its children.
  explicit GreenNode(SyntaxKind kind, GreenChild *children, size_t numChildren,
                     size_t width);

  /// Deleted default constructor to enforce proper initialization.
  GreenNode() = delete;

  /// \brief Get the syntax kind of this node.
  ///
  /// \return The SyntaxKind of this node.
  [[nodiscard]] SyntaxKind getKind() const noexcept { return data->kind; }

  /// \brief Get the width of this node.
  ///
  /// \return The total number of characters spanned by this node and its
  /// children.
  [[nodiscard]] size_t getWidth() const noexcept { return data->width; }

  /// \brief Get an iterator over this node's children.
  ///
  /// \return A GreenChildren iterator for traversing child elements.
  GreenChildren getChildren() const noexcept;

  /// \brief Get the number of children.
  ///
  /// \return The count of child elements in this node.
  [[nodiscard]] size_t getNumChildren() const noexcept {
    return data->numChildren;
  }

  /// \brief Get the reference count for this node's data.
  ///
  /// \return The number of references to the underlying data.
  [[nodiscard]] size_t getUseCount() const noexcept { return data.use_count(); }

  /// \brief Equality comparison operator.
  ///
  /// \param other The GreenNode to compare with.
  /// \return True if both nodes are structurally equal.
  bool operator==(const GreenNode &other) const noexcept;

  /// \brief Inequality comparison operator.
  ///
  /// \param other The GreenNode to compare with.
  /// \return True if the nodes are not structurally equal.
  bool operator!=(const GreenNode &other) const noexcept {
    return !(this == &other);
  }

private:
  std::shared_ptr<const GreenNodeData> data;
};

/// \brief A child element in a green node with its relative offset.
///
/// GreenChild represents a child element (either a node or token) within
/// a parent GreenNode, along with its relative offset from the start of
/// the parent. This allows efficient position calculation during tree
/// traversal.
struct [[nodiscard]] GreenChild final {
  /// The variant (node or token) for this child. GreenElement is specifically
  /// not used to avoid
  const std::variant<GreenNode, GreenToken> element;

  /// The offset of this child relative to its parent's start position.
  const size_t relativeOffset;

  /// \brief Equality comparison operator.
  ///
  /// \param other The GreenChild to compare with.
  /// \return True if both children have the same element and offset.
  bool operator==(const GreenChild &other) const {
    return element == other.element && relativeOffset == other.relativeOffset;
  }

  /// \brief Inequality comparison operator.
  ///
  /// \param other The GreenChild to compare with.
  /// \return True if the children differ in element or offset.
  bool operator!=(const GreenChild &other) const {
    return element != other.element || relativeOffset != other.relativeOffset;
  }
};

/// \brief A variant type representing either a GreenNode or GreenToken.
///
/// GreenElement is used when an element in the green tree could be either
/// a node or a token. It provides a unified interface for accessing common
/// properties and type-safe access to the underlying value.
class [[nodiscard]] GreenElement final : public std::variant<GreenNode, GreenToken> {
public:
  using std::variant<GreenNode, GreenToken>::variant;

  /// Deleted default constructor to enforce proper initialization.
  GreenElement() = delete;

  /// \brief Get the element as a GreenNode.
  ///
  /// \return Reference to the GreenNode.
  /// \pre The element must be a GreenNode (check with isNode()).
  [[nodiscard]] const GreenNode &getNode() const noexcept {
    return std::get<GreenNode>(*this);
  }

  /// \brief Get the element as a GreenNode pointer if it is one.
  ///
  /// \return Pointer to the GreenNode, or nullptr if this is a token.
  [[nodiscard]] const GreenNode *getIfNode() const noexcept {
    return std::get_if<GreenNode>(this);
  }

  /// \brief Get the element as a GreenToken.
  ///
  /// \return Reference to the GreenToken.
  /// \pre The element must be a GreenToken (check with isToken()).
  [[nodiscard]] const GreenToken &getToken() const noexcept {
    return std::get<GreenToken>(*this);
  }

  /// \brief Get the element as a GreenToken pointer if it is one.
  ///
  /// \return Pointer to the GreenToken, or nullptr if this is a node.
  [[nodiscard]] const GreenToken *getIfToken() const noexcept {
    return std::get_if<GreenToken>(this);
  }

  /// \brief Check if this element is a GreenNode.
  ///
  /// \return True if this element contains a GreenNode.
  [[nodiscard]] bool isNode() const noexcept {
    return std::holds_alternative<GreenNode>(*this);
  }

  /// \brief Check if this element is a GreenToken.
  ///
  /// \return True if this element contains a GreenToken.
  [[nodiscard]] bool isToken() const noexcept {
    return std::holds_alternative<GreenToken>(*this);
  }

  /// \brief Get the syntax kind of this element.
  ///
  /// \return The SyntaxKind of the underlying node or token.
  [[nodiscard]] SyntaxKind getKind() const noexcept {
    if (const GreenNode *node = getIfNode()) {
      return node->getKind();
    }

    if (const GreenToken *token = getIfToken()) {
      return token->getKind();
    }

    util::yuzu_unreachable();
  }

  /// \brief Get the width of this element.
  ///
  /// \return The number of characters spanned by this element.
  [[nodiscard]] size_t getWidth() const noexcept {
    if (const GreenNode *node = getIfNode()) {
      return node->getWidth();
    }

    if (const GreenToken *token = getIfToken()) {
      return token->getWidth();
    }

    util::yuzu_unreachable();
  }

  /// \brief Get the reference count for this element's data.
  ///
  /// \return The number of references to the underlying data.
  [[nodiscard]] size_t getUseCount() const noexcept {
    if (const GreenNode *node = getIfNode()) {
      return node->getUseCount();
    }

    if (const GreenToken *token = getIfToken()) {
      return token->getUseCount();
    }

    util::yuzu_unreachable();
  }
};
} // namespace yuzu::syntax

#endif // YUZU_SYNTAX_GREEN_GREEN_H
