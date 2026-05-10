#ifndef YUZU_SYNTAX_DETAIL_SYNTAX_H
#define YUZU_SYNTAX_DETAIL_SYNTAX_H

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/SyntaxKind.h"
#include "yuzu/Util/ErrorHandling.h"

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <optional>
#include <utility>
#include <variant>

namespace yuzu::syntax::detail {
class SyntaxChildren;
class SyntaxChildrenWithTokens;
class SyntaxElement;
class SyntaxNode;
class SyntaxToken;

/// \brief Internal data structure for syntax tree nodes and tokens.
///
/// SyntaxData is a reference-counted data structure that stores the actual
/// data for both SyntaxNode and SyntaxToken instances. It manages the
/// relationship between parent and child nodes, tracks position information,
/// and handles memory management through atomic reference counting.
///
/// This class is designed to be used internally by SyntaxNode and SyntaxToken
/// and should not be instantiated directly by users.
class [[nodiscard]] SyntaxData final {
public:
  friend class SyntaxNode;
  friend class SyntaxToken;

  /// \brief Construct a new SyntaxData instance.
  ///
  /// The reference count is initialized to 1, representing the single
  /// SyntaxNode or SyntaxToken that owns this instance.
  ///
  /// \param green The green element (node or token) backing this syntax data.
  /// \param parent Pointer to the parent SyntaxData, nullptr for root nodes.
  /// \param offset The absolute offset of this element in the source text.
  /// \param index The index of this element in its parent's children.
  explicit SyntaxData(const GreenElement &green, SyntaxData *const parent,
                      const size_t offset, const size_t index)
      : green(std::move(green)), parent(parent), offset(offset), index(index),
        rc(1) {}

  /// Deleted default constructor to enforce non-null invariant.
  SyntaxData() = delete;

  /// SyntaxData is not copyable. It is an internal reference-counted payload
  /// owned by exactly one SyntaxNode or SyntaxToken at a time and accessed
  /// elsewhere through raw pointers. Sharing is expressed by incrementing the
  /// reference count, not by duplicating the object.
  SyntaxData(const SyntaxData &) = delete;
  SyntaxData &operator=(const SyntaxData &) = delete;

  /// SyntaxData is not movable. Its address is stable and is stored as the
  /// parent pointer of child SyntaxData instances; moving it would invalidate
  /// those pointers.
  SyntaxData(SyntaxData &&) = delete;
  SyntaxData &operator=(SyntaxData &&) = delete;

  /// \brief Destructor.
  ///
  /// Trivially destroys this SyntaxData. Deletion only happens when the
  /// reference count has reached zero, which SyntaxNode and SyntaxToken
  /// check via decRc() before calling delete.
  ///
  /// The parent pointer is non-owning: children never outlive their parent
  /// (this is guaranteed by the tree's ownership invariant, since the root
  /// SyntaxNode keeps the entire red tree alive), so there is nothing to
  /// decrement or release here.
  ~SyntaxData() = default;

  /// \brief Get the reference count of this SyntaxData.
  ///
  /// \return The current atomic reference count.
  [[nodiscard]] int64_t getRc() const { return rc.load(); }

  /// \brief Increment the reference count.
  void incRc() { rc.fetch_add(1); }

  /// \brief Decrement the reference count.
  ///
  /// \return True if this was the last reference (count reached 0).
  [[nodiscard]] bool decRc() { return rc.fetch_sub(1) == 1; }

  /// \brief Get the green element backing this syntax data.
  ///
  /// \return The GreenElement (either GreenNode or GreenToken).
  [[nodiscard]] const GreenElement &getGreen() const { return green; }

  /// \brief Get the parent of this syntax data.
  ///
  /// \return Pointer to parent SyntaxData, or nullptr if this is a root.
  [[nodiscard]] const SyntaxData *getParent() const { return parent; }

  /// \brief Get the absolute offset in the source text.
  ///
  /// \return The offset in bytes from the start of the source.
  [[nodiscard]] size_t getOffset() const { return offset; }

  /// \brief Get the index of this element in its parent's children.
  ///
  /// \return The zero-based index.
  [[nodiscard]] size_t getIndex() const { return index; }

  /// \brief Get the next sibling node.
  ///
  /// \return The next SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getNextSibling() const;

  /// \brief Get the next sibling element (node or token).
  ///
  /// \return The next SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getNextSiblingOrToken() const;

  /// \brief Get the previous sibling node.
  ///
  /// \return The previous SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getPrevSibling() const;

  /// \brief Get the previous sibling element (node or token).
  ///
  /// \return The previous SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getPrevSiblingOrToken() const;

  /// \brief Equality comparison operator.
  ///
  /// Compares two SyntaxData instances by their green element and absolute
  /// offset. This is *positional* equality: two SyntaxData from different
  /// parse trees that occupy the same offset and are backed by structurally
  /// equal green elements compare equal, even though they have distinct
  /// parent chains and distinct SyntaxData addresses.
  ///
  /// This is intentional. Callers that want identity semantics should
  /// compare the underlying pointers directly, not use this operator.
  ///
  /// \param other The SyntaxData to compare with.
  /// \return True if both objects occupy the same position in structurally
  /// equal trees.
  bool operator==(const SyntaxData &other) const {
    return green == other.green && offset == other.offset;
  }

  /// \brief Inequality comparison operator.
  ///
  /// See `operator==` for the positional-equality semantics.
  ///
  /// \param other The SyntaxData to compare with.
  /// \return True if the objects differ in green element or offset.
  bool operator!=(const SyntaxData &other) const { return !(*this == other); }

private:
  /// The 'GreenElement' associated with this 'SyntaxData'. When parented to a
  /// 'SyntaxNode', this is a 'GreenNode'. When parented to a 'SyntaxToken',
  /// this is a 'GreenToken'.
  GreenElement green;

  /// The parent that this 'SyntaxNode' belongs to.
  ///
  /// This pointer is non-owning. Children never outlive their parent because
  /// the root SyntaxNode keeps the entire red tree alive, so it is safe to
  /// dereference without reference-count bookkeeping.
  SyntaxData *parent;

  /// The absolute offset of this 'SyntaxNode' in the source code.
  ///
  /// To illustrate this, in the code sample the character '(' has an absolute
  /// offset of 6.
  /// \code
  ///   print("hello world")
  /// \endcode
  size_t offset;

  /// The index of this 'SyntaxData' in the children of 'Parent'.
  size_t index;

  /// Reference count for the smart pointer to manage. Stored inline to avoid
  /// a second heap allocation per SyntaxData.
  std::atomic<int64_t> rc;
};

/// \brief A node in the concrete syntax tree.
///
/// SyntaxNode represents a non-terminal element in the syntax tree that can
/// contain child nodes and tokens. It provides methods to traverse the tree
/// structure and access node properties. This class uses reference counting
/// for memory management, similar to std::shared_ptr.
class [[nodiscard]] SyntaxNode final {
public:
  /// \brief Create a "root" SyntaxNode.
  ///
  /// Root nodes reference a GreenNode, have no parent, and are at the
  /// "start" of the syntax tree.
  ///
  /// \param node The GreenNode to build the root from.
  /// \return A root SyntaxNode.
  static SyntaxNode createRoot(GreenNode node) {
    return SyntaxNode(0, 0, nullptr, node);
  }

  /// \brief Construct a SyntaxNode.
  ///
  /// \param offset The absolute offset in the source text.
  /// \param idx The index in the parent's children.
  /// \param parent Pointer to the parent SyntaxData, or nullptr for root.
  /// \param green The GreenNode backing this syntax node.
  explicit SyntaxNode(size_t offset, size_t idx, SyntaxData *const parent,
                      GreenNode green)
      : data(new SyntaxData(green, parent, offset, idx)) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxNode() = delete;

  /// \brief Copy constructor.
  ///
  /// Creates a new reference to the same SyntaxNode and increments the
  /// reference count.
  ///
  /// \param other The SyntaxNode to copy from.
  SyntaxNode(const SyntaxNode &other) : data(other.data) { data->incRc(); }

  /// \brief Copy assignment operator.
  ///
  /// Assigns from another SyntaxNode, properly managing reference counts.
  /// Decrements the current reference count and increments the new one.
  ///
  /// \param other The SyntaxNode to assign from.
  /// \return Reference to this object.
  SyntaxNode &operator=(const SyntaxNode &other) {
    if (this == &other) {
      return *this;
    }

    // Delete the current pointer to Data if there are no references, since it
    // will be thrown away in favor of the new point. Failing to
    // delete this here will leak memory.
    if (data->decRc()) {
      delete data;
    }

    data = other.data;
    data->incRc();

    return *this;
  }

  /// \brief Destructor.
  ///
  /// Manages the destruction of the underlying SyntaxData, freeing it when
  /// there are no more references (similar to std::shared_ptr).
  ~SyntaxNode() {
    if (data->decRc()) {
      delete data;
    }
  }

  /// \brief Get the offset of this SyntaxNode.
  ///
  /// \return The absolute offset in bytes from the start of the source.
  [[nodiscard]] size_t getOffset() const { return data->getOffset(); }

  /// \brief Get the index of this SyntaxNode.
  ///
  /// \return The zero-based index in the parent's children.
  [[nodiscard]] size_t getIndex() const { return data->getIndex(); }

  /// \brief Get the parent of this SyntaxNode.
  ///
  /// It is safe to return a raw pointer, since the SyntaxData owns and
  /// maintains references.
  ///
  /// \return Pointer to parent SyntaxData, or nullptr if this is a root.
  [[nodiscard]] const SyntaxData *getParent() const {
    return data->getParent();
  }

  /// \brief Get the GreenNode of this SyntaxNode.
  ///
  /// For SyntaxNodes, the GreenElement backing it is always a GreenNode.
  ///
  /// \return Constant reference to the GreenNode.
  [[nodiscard]] const GreenNode &getGreen() const {
    return data->getGreen().getNode();
  }

  /// \brief Get the kind of this SyntaxNode.
  ///
  /// \return The SyntaxKind of this node.
  [[nodiscard]] SyntaxKind getKind() const {
    return data->getGreen().getKind();
  }

  /// \brief Get the reference count of this SyntaxNode.
  ///
  /// \return The current atomic reference count.
  [[nodiscard]] int64_t getRc() const { return data->getRc(); }

  /// \brief Get the children of this SyntaxNode.
  ///
  /// \return An iterator over child SyntaxNodes only (excludes tokens).
  SyntaxChildren getChildren() const;

  /// \brief Get the children of this SyntaxNode including tokens.
  ///
  /// \return An iterator over all child elements (nodes and tokens).
  SyntaxChildrenWithTokens getChildrenWithTokens() const;

  /// \brief Get the first child node.
  ///
  /// The first child is defined as the first child of this SyntaxNode that
  /// is a SyntaxNode (not a token).
  ///
  /// \return The first child SyntaxNode, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getFirstChild() const;

  /// \brief Get the first child element (node or token).
  ///
  /// The first child or token is defined as the first child of this
  /// SyntaxNode that is either a SyntaxNode or a SyntaxToken.
  ///
  /// \return The first child SyntaxElement, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getFirstChildOrToken() const;

  /// \brief Get the last child node.
  ///
  /// The last child is defined as the last child of this SyntaxNode that
  /// is a SyntaxNode (not a token).
  ///
  /// \return The last child SyntaxNode, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getLastChild() const;

  /// \brief Get the last child element (node or token).
  ///
  /// The last child or token is defined as the last child of this SyntaxNode
  /// that is either a SyntaxNode or a SyntaxToken.
  ///
  /// \return The last child SyntaxElement, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getLastChildOrToken() const;

  /// \brief Get the next sibling node.
  ///
  /// The next sibling is defined as the next node after this SyntaxNode in
  /// its parent's children.
  ///
  /// \return The next SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getNextSibling() const;

  /// \brief Get the next sibling element (node or token).
  ///
  /// The next sibling is defined as the next element after this SyntaxNode in
  /// its parent's children, which can be either a node or token.
  ///
  /// \return The next SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getNextSiblingOrToken() const;

  /// \brief Get the previous sibling node.
  ///
  /// The previous sibling is defined as the previous node before this
  /// SyntaxNode in its parent's children.
  ///
  /// \return The previous SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getPrevSibling() const;

  /// \brief Get the previous sibling element (node or token).
  ///
  /// The previous sibling is defined as the previous element before this
  /// SyntaxNode in its parent's children, which can be either a node or token.
  ///
  /// \return The previous SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getPrevSiblingOrToken() const;

  /// \brief Equality comparison operator.
  ///
  /// \param other The SyntaxNode to compare with.
  /// \return True if both nodes refer to the same underlying data.
  bool operator==(const SyntaxNode &other) const {
    return *data == *other.data;
  }

  /// \brief Inequality comparison operator.
  ///
  /// \param other The SyntaxNode to compare with.
  /// \return True if the nodes refer to different underlying data.
  bool operator!=(const SyntaxNode &other) const { return !(*this == other); }

private:
  SyntaxData *data;
};

/// \brief A token in the concrete syntax tree.
///
/// SyntaxToken represents a terminal element in the syntax tree (keywords,
/// identifiers, literals, operators, etc.). It provides methods to access
/// token properties and traverse the tree structure. This class uses
/// reference counting for memory management, similar to std::shared_ptr.
class [[nodiscard]] SyntaxToken final {
public:
  /// \brief Construct a SyntaxToken with a parent.
  ///
  /// \param offset The absolute offset in the source text.
  /// \param idx The index in the parent's children.
  /// \param parent Pointer to the parent SyntaxData.
  /// \param green The GreenToken backing this syntax token.
  explicit SyntaxToken(size_t offset, size_t idx, SyntaxData *const parent,
                       GreenToken green)
      : data(new SyntaxData(green, parent, offset, idx)) {}

  /// \brief Construct a SyntaxToken without a parent.
  ///
  /// \param offset The absolute offset in the source text.
  /// \param idx The index (typically 0 for parentless tokens).
  /// \param green The GreenToken backing this syntax token.
  explicit SyntaxToken(size_t offset, size_t idx, GreenToken green)
      : data(new SyntaxData(green, nullptr, offset, idx)) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxToken() = delete;

  /// \brief Copy constructor.
  ///
  /// Creates a new reference to the same SyntaxToken and increments the
  /// reference count.
  ///
  /// \param other The SyntaxToken to copy from.
  SyntaxToken(const SyntaxToken &other) : data(other.data) { data->incRc(); }

  /// \brief Copy assignment operator.
  ///
  /// Assigns from another SyntaxToken, properly managing reference counts.
  /// Decrements the current reference count and increments the new one.
  ///
  /// \param other The SyntaxToken to assign from.
  /// \return Reference to this object.
  SyntaxToken &operator=(const SyntaxToken &other) {
    if (this == &other) {
      return *this;
    }

    // Delete the current pointer to Data if there are no references, since it
    // will be thrown away in favor of the new point. Failing to
    // delete this here will leak memory.
    if (data->decRc()) {
      delete data;
    }

    data = other.data;
    data->incRc();

    return *this;
  }

  /// \brief Destructor.
  ///
  /// Manages the destruction of the underlying SyntaxData, freeing it when
  /// there are no more references (similar to std::shared_ptr).
  ~SyntaxToken() {
    if (data->decRc()) {
      delete data;
    }
  }

  /// \brief Get the offset of this SyntaxToken.
  ///
  /// \return The absolute offset in bytes from the start of the source.
  [[nodiscard]] size_t getOffset() const { return data->getOffset(); }

  /// \brief Get the index of this SyntaxToken.
  ///
  /// \return The zero-based index in the parent's children.
  [[nodiscard]] size_t getIndex() const { return data->getIndex(); }

  /// \brief Get the parent of this SyntaxToken.
  ///
  /// \return Pointer to parent SyntaxData, or nullptr if this token has no
  /// parent.
  [[nodiscard]] const SyntaxData *getParent() const {
    return data->getParent();
  }

  /// \brief Get the GreenToken of this SyntaxToken.
  ///
  /// For SyntaxTokens, the GreenElement backing it is always a GreenToken.
  ///
  /// \return Constant reference to the GreenToken.
  [[nodiscard]] const GreenToken &getGreen() const {
    return data->getGreen().getToken();
  }

  /// \brief Get the kind of this SyntaxToken.
  ///
  /// \return The SyntaxKind of this token.
  [[nodiscard]] SyntaxKind getKind() const {
    return data->getGreen().getKind();
  }

  /// \brief Get the reference count of this SyntaxToken.
  ///
  /// \return The current atomic reference count.
  [[nodiscard]] int64_t getRc() const { return data->getRc(); }

  /// \brief Get the next sibling node.
  ///
  /// \return The next SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getNextSibling() const;

  /// \brief Get the next sibling element (node or token).
  ///
  /// \return The next SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getNextSiblingOrToken() const;

  /// \brief Get the previous sibling node.
  ///
  /// \return The previous SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getPrevSibling() const;

  /// \brief Get the previous sibling element (node or token).
  ///
  /// \return The previous SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getPrevSiblingOrToken() const;

  /// \brief Equality comparison operator.
  ///
  /// \param other The SyntaxToken to compare with.
  /// \return True if both tokens refer to the same underlying data.
  bool operator==(const SyntaxToken &other) const {
    return *data == *other.data;
  }

  /// \brief Inequality comparison operator.
  ///
  /// \param other The SyntaxToken to compare with.
  /// \return True if the tokens refer to different underlying data.
  bool operator!=(const SyntaxToken &other) const { return !(*this == other); }

private:
  SyntaxData *data;
};

/// \brief A variant type representing either a SyntaxNode or SyntaxToken.
///
/// SyntaxElement is used when traversing the syntax tree and an element
/// could be either a node or a token. It provides methods to check the
/// type and access the underlying value safely.
class [[nodiscard]] SyntaxElement final
    : public std::variant<SyntaxNode, SyntaxToken> {
public:
  using std::variant<SyntaxNode, SyntaxToken>::variant;

  /// Deleted default constructor to enforce proper initialization.
  SyntaxElement() = delete;

  /// \brief Get the element as a SyntaxNode.
  ///
  /// \return Reference to the SyntaxNode.
  /// \pre The element must be a SyntaxNode (check with isNode()).
  [[nodiscard]] const SyntaxNode &getNode() const {
    return std::get<SyntaxNode>(*this);
  }

  /// \brief Get the element as a SyntaxNode pointer if it is one.
  ///
  /// \return Pointer to the SyntaxNode, or nullptr if this is a token.
  [[nodiscard]] const SyntaxNode *getIfNode() const {
    return std::get_if<SyntaxNode>(this);
  }

  /// \brief Get the element as a SyntaxToken.
  ///
  /// \return Reference to the SyntaxToken.
  /// \pre The element must be a SyntaxToken (check with isToken()).
  [[nodiscard]] const SyntaxToken &getToken() const {
    return std::get<SyntaxToken>(*this);
  }

  /// \brief Get the element as a SyntaxToken pointer if it is one.
  ///
  /// \return Pointer to the SyntaxToken, or nullptr if this is a node.
  [[nodiscard]] const SyntaxToken *getIfToken() const {
    return std::get_if<SyntaxToken>(this);
  }

  /// \brief Get the kind of this element.
  ///
  /// \return The SyntaxKind of the underlying node or token.
  [[nodiscard]] SyntaxKind getKind() const {
    if (const SyntaxNode *node = getIfNode()) {
      return node->getKind();
    }

    if (const SyntaxToken *token = getIfToken()) {
      return token->getKind();
    }

    util::yuzu_unreachable();
  }

  /// \brief Check if this element is a SyntaxNode.
  ///
  /// \return True if this element contains a SyntaxNode.
  [[nodiscard]] bool isNode() const {
    return std::holds_alternative<SyntaxNode>(*this);
  }

  /// \brief Check if this element is a SyntaxToken.
  ///
  /// \return True if this element contains a SyntaxToken.
  [[nodiscard]] bool isToken() const {
    return std::holds_alternative<SyntaxToken>(*this);
  }

  /// \brief Get the next sibling node.
  ///
  /// \return The next SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getNextSibling() const {
    if (const SyntaxNode *node = getIfNode()) {
      return node->getNextSibling();
    }

    if (const SyntaxToken *token = getIfToken()) {
      return token->getNextSibling();
    }

    util::yuzu_unreachable();
  }

  /// \brief Get the next sibling element (node or token).
  ///
  /// \return The next SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getNextSiblingOrToken() const {
    if (const SyntaxNode *node = getIfNode()) {
      return node->getNextSiblingOrToken();
    }

    if (const SyntaxToken *token = getIfToken()) {
      return token->getNextSiblingOrToken();
    }

    util::yuzu_unreachable();
  }

  /// \brief Get the previous sibling node.
  ///
  /// \return The previous SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getPrevSibling() const {
    if (const SyntaxNode *node = getIfNode()) {
      return node->getPrevSibling();
    }

    if (const SyntaxToken *token = getIfToken()) {
      return token->getPrevSibling();
    }

    util::yuzu_unreachable();
  }

  /// \brief Get the previous sibling element (node or token).
  ///
  /// \return The previous SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getPrevSiblingOrToken() const {
    if (const SyntaxNode *node = getIfNode()) {
      return node->getPrevSiblingOrToken();
    }

    if (const SyntaxToken *token = getIfToken()) {
      return token->getPrevSiblingOrToken();
    }

    util::yuzu_unreachable();
  }
};

/// \brief Bidirectional iterator for traversing sibling SyntaxNodes.
///
/// SyntaxIterator provides a standard C++ iterator interface for iterating
/// over sibling nodes in the syntax tree. It traverses only SyntaxNode
/// elements, skipping tokens. The iterator uses std::optional to represent
/// the end state.
class [[nodiscard]] SyntaxIterator final {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxNode;
  using pointer = value_type *;
  using reference = value_type &;

  explicit SyntaxIterator(std::optional<SyntaxNode> current)
      : current(current) {}

  SyntaxIterator() = delete;

  reference operator*() const { return current.value(); }

  pointer operator->() const { return &current.value(); }

  SyntaxIterator &operator++() {
    if (current.has_value()) {
      current = current->getNextSibling();
    }

    return *this;
  }

  SyntaxIterator operator++(int) {
    SyntaxIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  SyntaxIterator &operator--() {
    if (current.has_value()) {
      current = current->getPrevSibling();
    }

    return *this;
  }

  SyntaxIterator operator--(int) {
    SyntaxIterator tmp = *this;
    --(*this);
    return tmp;
  }

  friend bool operator==(const SyntaxIterator &a, const SyntaxIterator &b) {
    return a.current == b.current;
  }

  friend bool operator!=(const SyntaxIterator &a, const SyntaxIterator &b) {
    return !(a == b);
  }

private:
  std::optional<SyntaxNode> current;
};

/// \brief Bidirectional iterator for traversing sibling SyntaxElements.
///
/// SyntaxIteratorWithTokens provides a standard C++ iterator interface for
/// iterating over sibling elements in the syntax tree, including both nodes
/// and tokens. The iterator uses std::optional to represent the end state.
class [[nodiscard]] SyntaxIteratorWithTokens final {
public:
  using iterator_category = std::bidirectional_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxElement;
  using pointer = value_type *;
  using reference = value_type &;

  explicit SyntaxIteratorWithTokens(std::optional<SyntaxElement> current)
      : current(current) {}

  SyntaxIteratorWithTokens() = delete;

  reference operator*() const { return current.value(); }

  pointer operator->() const { return &current.value(); }

  SyntaxIteratorWithTokens &operator++() {
    if (current.has_value()) {
      current = current->getNextSiblingOrToken();
    }

    return *this;
  }

  SyntaxIteratorWithTokens operator++(int) {
    SyntaxIteratorWithTokens tmp = std::move(*this);
    ++(*this);
    return tmp;
  }

  SyntaxIteratorWithTokens &operator--() {
    if (current.has_value()) {
      current = current->getPrevSiblingOrToken();
    }

    return *this;
  }

  SyntaxIteratorWithTokens operator--(int) {
    SyntaxIteratorWithTokens tmp = std::move(*this);
    --(*this);
    return tmp;
  }

  friend bool operator==(const SyntaxIteratorWithTokens &a,
                         const SyntaxIteratorWithTokens &b) {
    return a.current == b.current;
  }

  friend bool operator!=(const SyntaxIteratorWithTokens &a,
                         const SyntaxIteratorWithTokens &b) {
    return !(a == b);
  }

private:
  std::optional<SyntaxElement> current;
};

/// \brief Range wrapper for iterating over SyntaxNode children.
///
/// SyntaxChildren provides a standard C++ range interface for accessing
/// the child nodes of a SyntaxNode. It supports both forward and reverse
/// iteration using SyntaxIterator, traversing only node children and
/// skipping tokens.
class [[nodiscard]] SyntaxChildren final {
public:
  using const_iterator = SyntaxIterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = typename const_iterator::value_type;

  explicit SyntaxChildren(const SyntaxNode *node) : node(node) {}

  SyntaxChildren() = delete;

  const_iterator begin() const { return const_iterator(node->getFirstChild()); }

  const_iterator end() const { return const_iterator(std::nullopt); }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(const_iterator(node->getLastChild()));
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  const SyntaxNode *const node;
};

/// \brief Range wrapper for iterating over SyntaxNode children and tokens.
///
/// SyntaxChildrenWithTokens provides a standard C++ range interface for
/// accessing all child elements of a SyntaxNode, including both nodes and
/// tokens. It supports both forward and reverse iteration using
/// SyntaxIteratorWithTokens.
class [[nodiscard]] SyntaxChildrenWithTokens final {
public:
  using const_iterator = SyntaxIteratorWithTokens;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using value_type = const_iterator::value_type;

  explicit SyntaxChildrenWithTokens(const SyntaxNode *node) : node(node) {}

  SyntaxChildrenWithTokens() = delete;

  const_iterator begin() const {
    return const_iterator(node->getFirstChildOrToken());
  }

  const_iterator end() const { return const_iterator(std::nullopt); }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(const_iterator(node->getLastChildOrToken()));
  }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(const_iterator(std::nullopt));
  }

private:
  const SyntaxNode *const node;
};
} // namespace yuzu::syntax::detail

#endif // YUZU_SYNTAX_DETAIL_SYNTAX_H
