#ifndef YUZU_SYNTAX_API_H
#define YUZU_SYNTAX_API_H

#include "yuzu/Syntax/Syntax.h"
#include "yuzu/Util/ErrorHandling.h"

#include <cstddef>
#include <iterator>
#include <optional>
#include <utility>
#include <variant>

namespace yuzu::syntax::api {
// Forward Declarations.
template <typename Kind> class SyntaxNode;
template <typename Kind> class SyntaxToken;
template <typename Kind> class SyntaxElement;
template <typename Kind> class SyntaxIterator;
template <typename Kind> class SyntaxIteratorWithTokens;
template <typename Kind> class SyntaxChildren;
template <typename Kind> class SyntaxChildrenWithTokens;

/// \brief A typed view of a node in the concrete syntax tree.
///
/// SyntaxNode is a thin forwarder around syntax::SyntaxNode that exposes a
/// frontend-supplied Kind enum (typically a generated `enum class`) instead
/// of the untyped uint16_t carried by the core. The only behavioural
/// difference from the core is that getKind() returns Kind.
template <typename Kind> class [[nodiscard]] SyntaxNode final {
public:
  /// \brief Create a "root" SyntaxNode.
  ///
  /// Root nodes reference a GreenNode, have no parent, and are at the
  /// "start" of the syntax tree.
  ///
  /// \param node The GreenNode to build the root from.
  /// \return A root SyntaxNode.
  static SyntaxNode createRoot(GreenNode node) {
    return SyntaxNode(syntax::SyntaxNode::createRoot(std::move(node)));
  }

  /// \brief Wrap an existing untyped core SyntaxNode.
  ///
  /// \param raw The core SyntaxNode to wrap.
  explicit SyntaxNode(syntax::SyntaxNode raw) : raw(std::move(raw)) {}

  /// Deleted default constructor: the underlying core type is non-default.
  SyntaxNode() = delete;

  /// \brief Get the offset of this SyntaxNode.
  ///
  /// \return The absolute offset in bytes from the start of the source.
  [[nodiscard]] size_t getOffset() const { return raw.getOffset(); }

  /// \brief Get the index of this SyntaxNode.
  ///
  /// \return The zero-based index in the parent's children.
  [[nodiscard]] size_t getIndex() const { return raw.getIndex(); }

  /// \brief Get the parent of this SyntaxNode.
  ///
  /// It is safe to return a raw pointer, since the SyntaxData owns and
  /// maintains references.
  ///
  /// \return Pointer to parent SyntaxData, or nullptr if this is a root.
  [[nodiscard]] const SyntaxData *getParent() const { return raw.getParent(); }

  /// \brief Get the GreenNode of this SyntaxNode.
  ///
  /// For SyntaxNodes, the GreenElement backing it is always a GreenNode.
  ///
  /// \return Constant reference to the GreenNode.
  [[nodiscard]] const GreenNode &getGreen() const { return raw.getGreen(); }

  /// \brief Get the typed kind of this SyntaxNode.
  ///
  /// \return The Kind value, cast from the underlying uint16_t.
  [[nodiscard]] Kind getKind() const {
    return static_cast<Kind>(raw.getKind());
  }

  /// \brief Get the reference count of this SyntaxNode.
  ///
  /// \return The current atomic reference count.
  [[nodiscard]] int64_t getRc() const { return raw.getRc(); }

  /// \brief Get the children of this SyntaxNode.
  ///
  /// \return A range over child SyntaxNodes only (excludes tokens).
  [[nodiscard]] SyntaxChildren<Kind> getChildren() const {
    return SyntaxChildren<Kind>(raw.getChildren());
  }

  /// \brief Get the children of this SyntaxNode including tokens.
  ///
  /// \return A range over all child elements (nodes and tokens).
  [[nodiscard]] SyntaxChildrenWithTokens<Kind> getChildrenWithTokens() const {
    return SyntaxChildrenWithTokens<Kind>(raw.getChildrenWithTokens());
  }

  /// \brief Get the first child node.
  ///
  /// \return The first child SyntaxNode, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getFirstChild() const {
    if (auto child = raw.getFirstChild()) {
      return SyntaxNode(*std::move(child));
    }
    return std::nullopt;
  }

  /// \brief Get the first child element (node or token).
  ///
  /// \return The first child SyntaxElement, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement<Kind>>
  getFirstChildOrToken() const {
    if (auto child = raw.getFirstChildOrToken()) {
      return SyntaxElement<Kind>(*std::move(child));
    }
    return std::nullopt;
  }

  /// \brief Get the last child node.
  ///
  /// \return The last child SyntaxNode, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getLastChild() const {
    if (auto child = raw.getLastChild()) {
      return SyntaxNode(*std::move(child));
    }
    return std::nullopt;
  }

  /// \brief Get the last child element (node or token).
  ///
  /// \return The last child SyntaxElement, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement<Kind>> getLastChildOrToken() const {
    if (auto child = raw.getLastChildOrToken()) {
      return SyntaxElement<Kind>(*std::move(child));
    }
    return std::nullopt;
  }

  /// \brief Get the next sibling node.
  ///
  /// \return The next SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getNextSibling() const {
    if (auto sibling = raw.getNextSibling()) {
      return SyntaxNode(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Get the next sibling element (node or token).
  ///
  /// \return The next SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement<Kind>>
  getNextSiblingOrToken() const {
    if (auto sibling = raw.getNextSiblingOrToken()) {
      return SyntaxElement<Kind>(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Get the previous sibling node.
  ///
  /// \return The previous SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode> getPrevSibling() const {
    if (auto sibling = raw.getPrevSibling()) {
      return SyntaxNode(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Get the previous sibling element (node or token).
  ///
  /// \return The previous SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement<Kind>>
  getPrevSiblingOrToken() const {
    if (auto sibling = raw.getPrevSiblingOrToken()) {
      return SyntaxElement<Kind>(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Equality comparison: positional, via the core's operator==.
  bool operator==(const SyntaxNode &other) const { return raw == other.raw; }

  /// \brief Inequality comparison.
  bool operator!=(const SyntaxNode &other) const { return raw != other.raw; }

private:
  // SyntaxElement<Kind> needs to read this when wrapping a SyntaxNode<Kind>
  // into a typed element; no public escape hatch is exposed.
  friend class SyntaxElement<Kind>;

  syntax::SyntaxNode raw;
};

/// \brief A typed view of a token in the concrete syntax tree.
///
/// Mirrors syntax::SyntaxToken with getKind() returning the frontend's Kind
/// enum rather than the underlying uint16_t.
template <typename Kind> class [[nodiscard]] SyntaxToken final {
public:
  /// \brief Wrap an existing untyped core SyntaxToken.
  ///
  /// \param raw The core SyntaxToken to wrap.
  explicit SyntaxToken(syntax::SyntaxToken raw) : raw(std::move(raw)) {}

  /// Deleted default constructor: the underlying core type is non-default.
  SyntaxToken() = delete;

  /// \brief Get the offset of this SyntaxToken.
  ///
  /// \return The absolute offset in bytes from the start of the source.
  [[nodiscard]] size_t getOffset() const { return raw.getOffset(); }

  /// \brief Get the index of this SyntaxToken.
  ///
  /// \return The zero-based index in the parent's children.
  [[nodiscard]] size_t getIndex() const { return raw.getIndex(); }

  /// \brief Get the parent of this SyntaxToken.
  ///
  /// \return Pointer to parent SyntaxData, or nullptr if this token has no
  /// parent.
  [[nodiscard]] const SyntaxData *getParent() const { return raw.getParent(); }

  /// \brief Get the GreenToken backing this SyntaxToken.
  ///
  /// \return Constant reference to the GreenToken.
  [[nodiscard]] const GreenToken &getGreen() const { return raw.getGreen(); }

  /// \brief Get the typed kind of this SyntaxToken.
  ///
  /// \return The Kind value, cast from the underlying uint16_t.
  [[nodiscard]] Kind getKind() const {
    return static_cast<Kind>(raw.getKind());
  }

  /// \brief Get the reference count of this SyntaxToken.
  ///
  /// \return The current atomic reference count.
  [[nodiscard]] int64_t getRc() const { return raw.getRc(); }

  /// \brief Get the next sibling node.
  ///
  /// \return The next SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode<Kind>> getNextSibling() const {
    if (auto sibling = raw.getNextSibling()) {
      return SyntaxNode<Kind>(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Get the next sibling element (node or token).
  ///
  /// \return The next SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement<Kind>>
  getNextSiblingOrToken() const {
    if (auto sibling = raw.getNextSiblingOrToken()) {
      return SyntaxElement<Kind>(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Get the previous sibling node.
  ///
  /// \return The previous SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode<Kind>> getPrevSibling() const {
    if (auto sibling = raw.getPrevSibling()) {
      return SyntaxNode<Kind>(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Get the previous sibling element (node or token).
  ///
  /// \return The previous SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement<Kind>>
  getPrevSiblingOrToken() const {
    if (auto sibling = raw.getPrevSiblingOrToken()) {
      return SyntaxElement<Kind>(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Equality comparison: positional, via the core's operator==.
  bool operator==(const SyntaxToken &other) const { return raw == other.raw; }

  /// \brief Inequality comparison.
  bool operator!=(const SyntaxToken &other) const { return raw != other.raw; }

private:
  // SyntaxElement<Kind> needs to read this when wrapping a SyntaxToken<Kind>
  // into a typed element; no public escape hatch is exposed.
  friend class SyntaxElement<Kind>;

  syntax::SyntaxToken raw;
};

/// \brief A typed variant of either a SyntaxNode<Kind> or SyntaxToken<Kind>.
///
/// Inherits from std::variant so std::visit / std::get / std::get_if work as
/// expected. Additionally holds the underlying syntax::SyntaxElement so that
/// scalar accessors (kind, isNode/isToken, sibling navigation) forward to
/// the core directly instead of dispatching across the variant alternatives.
template <typename Kind>
class [[nodiscard]] SyntaxElement final
    : public std::variant<SyntaxNode<Kind>, SyntaxToken<Kind>> {
public:
  using std::variant<SyntaxNode<Kind>, SyntaxToken<Kind>>::variant;

  /// \brief Wrap an existing untyped core SyntaxElement.
  ///
  /// The variant base is initialized to the matching typed alternative; the
  /// `raw` field holds the original core value for forwarding accessors.
  ///
  /// \param core The core SyntaxElement to wrap.
  explicit SyntaxElement(syntax::SyntaxElement core)
      : std::variant<SyntaxNode<Kind>, SyntaxToken<Kind>>(
            [&]() -> std::variant<SyntaxNode<Kind>, SyntaxToken<Kind>> {
              if (const auto *node = core.getIfNode()) {
                return SyntaxNode<Kind>(*node);
              }
              if (const auto *token = core.getIfToken()) {
                return SyntaxToken<Kind>(*token);
              }
              util::yuzu_unreachable();
            }()),
        raw(std::move(core)) {}

  /// \brief Build a SyntaxElement from a typed SyntaxNode.
  ///
  /// \param node The SyntaxNode<Kind> to lift into a SyntaxElement.
  explicit SyntaxElement(SyntaxNode<Kind> node)
      : std::variant<SyntaxNode<Kind>, SyntaxToken<Kind>>(node),
        raw(syntax::SyntaxElement(node.raw)) {}

  /// \brief Build a SyntaxElement from a typed SyntaxToken.
  ///
  /// \param token The SyntaxToken<Kind> to lift into a SyntaxElement.
  explicit SyntaxElement(SyntaxToken<Kind> token)
      : std::variant<SyntaxNode<Kind>, SyntaxToken<Kind>>(token),
        raw(syntax::SyntaxElement(token.raw)) {}

  /// Deleted default constructor: alternatives are non-default.
  SyntaxElement() = delete;

  /// \brief Get the element as a SyntaxNode<Kind>.
  ///
  /// \pre The element must be a SyntaxNode (check with isNode()).
  [[nodiscard]] const SyntaxNode<Kind> &getNode() const {
    return std::get<SyntaxNode<Kind>>(*this);
  }

  /// \brief Get the element as a SyntaxNode<Kind> pointer if it is one.
  [[nodiscard]] const SyntaxNode<Kind> *getIfNode() const {
    return std::get_if<SyntaxNode<Kind>>(this);
  }

  /// \brief Get the element as a SyntaxToken<Kind>.
  ///
  /// \pre The element must be a SyntaxToken (check with isToken()).
  [[nodiscard]] const SyntaxToken<Kind> &getToken() const {
    return std::get<SyntaxToken<Kind>>(*this);
  }

  /// \brief Get the element as a SyntaxToken<Kind> pointer if it is one.
  [[nodiscard]] const SyntaxToken<Kind> *getIfToken() const {
    return std::get_if<SyntaxToken<Kind>>(this);
  }

  /// \brief Get the typed kind of this element.
  ///
  /// \return The Kind value, cast from the underlying uint16_t.
  [[nodiscard]] Kind getKind() const {
    return static_cast<Kind>(raw.getKind());
  }

  /// \brief Check if this element holds a SyntaxNode<Kind>.
  [[nodiscard]] bool isNode() const { return raw.isNode(); }

  /// \brief Check if this element holds a SyntaxToken<Kind>.
  [[nodiscard]] bool isToken() const { return raw.isToken(); }

  /// \brief Get the next sibling node.
  ///
  /// \return The next SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode<Kind>> getNextSibling() const {
    if (auto sibling = raw.getNextSibling()) {
      return SyntaxNode<Kind>(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Get the next sibling element (node or token).
  ///
  /// \return The next SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getNextSiblingOrToken() const {
    if (auto sibling = raw.getNextSiblingOrToken()) {
      return SyntaxElement(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Get the previous sibling node.
  ///
  /// \return The previous SyntaxNode sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxNode<Kind>> getPrevSibling() const {
    if (auto sibling = raw.getPrevSibling()) {
      return SyntaxNode<Kind>(*std::move(sibling));
    }
    return std::nullopt;
  }

  /// \brief Get the previous sibling element (node or token).
  ///
  /// \return The previous SyntaxElement sibling, or nullopt if none exists.
  [[nodiscard]] std::optional<SyntaxElement> getPrevSiblingOrToken() const {
    if (auto sibling = raw.getPrevSiblingOrToken()) {
      return SyntaxElement(*std::move(sibling));
    }
    return std::nullopt;
  }

private:
  syntax::SyntaxElement raw;
};

/// \brief Forward-only iterator over typed sibling SyntaxNodes.
///
/// Wraps syntax::SyntaxIterator and materializes a SyntaxNode<Kind> on each
/// dereference. Forward-only because operator* returns by value: there is
/// no stable storage to back a `const SyntaxNode<Kind>&` reference, so we
/// declare input-iterator semantics and skip std::reverse_iterator support.
template <typename Kind> class [[nodiscard]] SyntaxIterator final {
public:
  using iterator_category = std::input_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = SyntaxNode<Kind>;
  using pointer = void;
  using reference = SyntaxNode<Kind>;

  /// \brief Wrap a core SyntaxIterator at the given position.
  explicit SyntaxIterator(syntax::SyntaxIterator raw) : raw(std::move(raw)) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxIterator() = delete;

  /// \brief Dereference: materialize the current node as SyntaxNode<Kind>.
  SyntaxNode<Kind> operator*() const { return SyntaxNode<Kind>(*raw); }

  /// \brief Pre-increment: advance to the next sibling node.
  SyntaxIterator &operator++() {
    ++raw;
    return *this;
  }

  /// \brief Post-increment.
  SyntaxIterator operator++(int) {
    SyntaxIterator tmp = *this;
    ++raw;
    return tmp;
  }

  /// \brief Equality comparison.
  friend bool operator==(const SyntaxIterator &a, const SyntaxIterator &b) {
    return a.raw == b.raw;
  }

  /// \brief Inequality comparison.
  friend bool operator!=(const SyntaxIterator &a, const SyntaxIterator &b) {
    return !(a == b);
  }

private:
  syntax::SyntaxIterator raw;
};

/// \brief Forward-only iterator over typed sibling SyntaxElements (nodes
/// and tokens). Wraps syntax::SyntaxIteratorWithTokens; see SyntaxIterator
/// for the rationale behind the input-iterator/by-value design.
template <typename Kind> class [[nodiscard]] SyntaxIteratorWithTokens final {
public:
  using iterator_category = std::input_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = SyntaxElement<Kind>;
  using pointer = void;
  using reference = SyntaxElement<Kind>;

  /// \brief Wrap a core SyntaxIteratorWithTokens at the given position.
  explicit SyntaxIteratorWithTokens(syntax::SyntaxIteratorWithTokens raw)
      : raw(std::move(raw)) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxIteratorWithTokens() = delete;

  /// \brief Dereference: materialize the current element as
  /// SyntaxElement<Kind>.
  SyntaxElement<Kind> operator*() const { return SyntaxElement<Kind>(*raw); }

  SyntaxIteratorWithTokens &operator++() {
    ++raw;
    return *this;
  }

  SyntaxIteratorWithTokens operator++(int) {
    SyntaxIteratorWithTokens tmp = *this;
    ++raw;
    return tmp;
  }

  friend bool operator==(const SyntaxIteratorWithTokens &a,
                         const SyntaxIteratorWithTokens &b) {
    return a.raw == b.raw;
  }

  friend bool operator!=(const SyntaxIteratorWithTokens &a,
                         const SyntaxIteratorWithTokens &b) {
    return !(a == b);
  }

private:
  syntax::SyntaxIteratorWithTokens raw;
};

/// \brief Forward range over a SyntaxNode<Kind>'s child nodes (skipping
/// tokens). Wraps syntax::SyntaxChildren; reverse iteration is intentionally
/// not supported (see SyntaxIterator).
template <typename Kind> class [[nodiscard]] SyntaxChildren final {
public:
  using const_iterator = SyntaxIterator<Kind>;
  using value_type = typename const_iterator::value_type;

  /// \brief Wrap a core SyntaxChildren range.
  explicit SyntaxChildren(syntax::SyntaxChildren raw) : raw(std::move(raw)) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxChildren() = delete;

  const_iterator begin() const { return const_iterator(raw.begin()); }
  const_iterator end() const { return const_iterator(raw.end()); }

private:
  syntax::SyntaxChildren raw;
};

/// \brief Forward range over a SyntaxNode<Kind>'s child elements (nodes and
/// tokens). Wraps syntax::SyntaxChildrenWithTokens.
template <typename Kind> class [[nodiscard]] SyntaxChildrenWithTokens final {
public:
  using const_iterator = SyntaxIteratorWithTokens<Kind>;
  using value_type = typename const_iterator::value_type;

  /// \brief Wrap a core SyntaxChildrenWithTokens range.
  explicit SyntaxChildrenWithTokens(syntax::SyntaxChildrenWithTokens raw)
      : raw(std::move(raw)) {}

  /// Deleted default constructor to enforce proper initialization.
  SyntaxChildrenWithTokens() = delete;

  const_iterator begin() const { return const_iterator(raw.begin()); }
  const_iterator end() const { return const_iterator(raw.end()); }

private:
  syntax::SyntaxChildrenWithTokens raw;
};

} // namespace yuzu::syntax::api

#endif // YUZU_SYNTAX_API_H
