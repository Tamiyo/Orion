#ifndef YUZU_AST_AST_H
#define YUZU_AST_AST_H

#include "yuzu/Ast/Syntax.h"
#include "yuzu/Syntax/Syntax.h"
#include "yuzu/Syntax/SyntaxIterator.h"

#include <memory>
#include <optional>
#include <utility>

namespace yuzu::ast {

/// \brief Base class for AST nodes using the Curiously Recurring Template
/// Pattern (CRTP).
///
/// This class provides a common interface for all AST node types in the Yuzu
/// language. Subclasses should inherit from AstNode<DerivedType> and implement
/// the static canCast() and cast() methods.
///
/// \tparam Self The derived class type that inherits from this base class.
template <typename Self> class AstNode {
public:
  virtual ~AstNode() = default;

  /// Copy constructor is deleted - AST nodes cannot be copied.
  AstNode(const AstNode &) = delete;

  /// Copy assignment operator is deleted - AST nodes cannot be copied.
  AstNode &operator=(const AstNode &) = delete;

  /// Move constructor is defaulted - AST nodes can be moved.
  AstNode(AstNode &&) = default;

  /// Move assignment operator is defaulted - AST nodes can be moved.
  AstNode &operator=(AstNode &&) = default;

  /// \brief Check if a syntax node of the given kind can be cast to this AST
  /// node type.
  ///
  /// \param Kind The syntax kind to check.
  /// \return True if a node of the given kind can be cast to Self.
  [[nodiscard]] static bool canCast(SyntaxKind Kind) noexcept {
    return Self::canCast(Kind);
  }

  /// \brief Attempt to cast a syntax node to this AST node type.
  ///
  /// \param Node The syntax node to cast.
  /// \return An optional containing the AST node if the cast succeeds, or
  ///         std::nullopt if the cast fails.
  [[nodiscard]] static std::optional<Self>
  cast(syntax::SyntaxNode Node) noexcept {
    return Self::cast(Node);
  }

protected:
  /// \brief Construct an AST node from a syntax node.
  ///
  /// \param Node The underlying syntax node.
  explicit AstNode(syntax::SyntaxNode Node) : Node_(std::move(Node)) {}

  /// The underlying syntax node that this AST node wraps.
  const syntax::SyntaxNode Node_;
};

/// \brief Type trait to check if a type is derived from any instantiation of
/// AstNode.
///
/// This trait uses SFINAE to determine if a given type Self is derived from
/// AstNode<T> for some type T.
///
/// \tparam Self The type to check for AstNode derivation.
template <typename Self> struct IsAstSubclass {
private:
  /// Overload selected if Self* is convertible to AstNode<T>* for some T.
  template <typename T> static std::true_type check(AstNode<T> *);

  /// Fallback overload selected if Self* is not convertible to any AstNode<T>*.
  static std::false_type check(...);

public:
  /// True if Self is derived from AstNode<T> for some T, false otherwise.
  static constexpr bool value = decltype(check(std::declval<Self *>()))::value;
};

/// \brief Type trait to check if a type has a valid static cast method.
///
/// This trait verifies that type T has a static method 'cast' that takes a
/// syntax::SyntaxNode and returns a type convertible to std::optional<T>.
///
/// \tparam T The type to check for a valid cast method.
template <typename T, typename = void>
struct HasStaticCastMethod : std::false_type {};

/// \brief Specialization of HasStaticCastMethod for types with valid cast
/// methods.
///
/// \tparam T The type to check for a valid cast method.
template <typename T>
struct HasStaticCastMethod<
    T,
    /// SFINAE expression using std::void_t to check if the inner types are
    /// valid.
    std::void_t<
        /// 1. Check if T has a static member named 'cast'.
        decltype(T::cast(std::declval<syntax::SyntaxNode>())),
        /// 2. Check if the return type of 'cast' is convertible to
        /// std::optional<T>.
        typename std::enable_if<std::is_convertible<
            decltype(T::cast(std::declval<syntax::SyntaxNode>())),
            std::optional<T>>::value>::type>> : std::true_type {};

/// \brief Get the Nth child of a specific AST node type from a syntax node.
///
/// This function searches through the children of the given syntax node and
/// returns the Nth occurrence of a child that can be cast to type T.
///
/// \tparam T The AST node type to search for. Must have a valid static cast
///           method.
/// \param Node The syntax node whose children to search.
/// \param N The zero-based index of the child to retrieve (default: 0).
/// \return A unique_ptr to the found child, or nullptr if not found.
template <typename T>
[[nodiscard]] inline std::unique_ptr<T> child(syntax::SyntaxNode Node,
                                              size_t N = 0) noexcept {
  static_assert(HasStaticCastMethod<T>::value,
                "T must be a subclass of AstNode<T> for some type T");

  size_t Count = 0;
  const syntax::SyntaxChildren Children = Node.getChildren();
  for (const auto &Child : Children) {
    std::optional<T> CastNode = T::cast(Child);
    if (CastNode.has_value() && Count == N) {
      return std::make_unique<T>(std::move(CastNode.value()));
    } else if (CastNode.has_value() && Count != N) {
      Count += 1;
    }
  }

  return nullptr;
}

/// \brief Get the Nth token of a specific kind from a syntax node.
///
/// This function searches through the children (including tokens) of the given
/// syntax node and returns the Nth occurrence of a token matching the specified
/// kind.
///
/// \param Node The syntax node whose children to search.
/// \param Kind The syntax kind of the token to find.
/// \param N The zero-based index of the token to retrieve.
/// \return An optional containing the token if found, or std::nullopt if not
///         found.
[[nodiscard]] inline std::optional<syntax::SyntaxToken>
token(syntax::SyntaxNode Node, SyntaxKind Kind, size_t N) noexcept {
  size_t Count = 0;
  const syntax::SyntaxChildrenWithTokens Children =
      Node.getChildrenWithTokens();
  for (const auto &Child : Children) {
    const bool IsSameKind = static_cast<SyntaxKind>(Child.getKind()) == Kind;

    if (IsSameKind && Count == N) {
      return Child.getToken();
    } else if (IsSameKind && Count != N) {
      Count += 1;
    }
  }

  return std::nullopt;
}
} // namespace yuzu::ast

#endif // YUZU_AST_AST_H
