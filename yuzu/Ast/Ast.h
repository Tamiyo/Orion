#ifndef YUZU_AST_AST_H
#define YUZU_AST_AST_H

#include "yuzu/Ast/SyntaxKind.h"
#include "yuzu/Syntax/Syntax.h"
#include "yuzu/Syntax/SyntaxIterator.h"

#include <optional>

namespace yuzu::ast {
template <typename Self> class AstNode {
public:
  virtual ~AstNode() = default;
  AstNode(const AstNode &) = delete;
  AstNode &operator=(const AstNode &) = delete;
  AstNode(AstNode &&) = default;
  AstNode &operator=(AstNode &&) = default;

  [[nodiscard]] static bool canCast(SyntaxKind Kind) noexcept {
    return Self::canCast(Kind);
  }

  [[nodiscard]] static std::optional<Self>
  cast(syntax::SyntaxNode Node) noexcept {
    return Self::cast(Node);
  }

protected:
  explicit AstNode(syntax::SyntaxNode Node) : Node_(std::move(Node)) {}

  const syntax::SyntaxNode Node_;
};

template <typename T>
[[nodiscard]] std::optional<AstNode<T>> child(syntax::SyntaxNode Node,
                                              size_t N = 0) noexcept {
  size_t Count = 0;
  for (const auto &Child : Node.getChildren()) {
    const auto CastNode = AstNode<T>::cast(Child.getKind());
    if (CastNode.has_value() && Count == N) {
      return CastNode.value();
    } else if (CastNode.has_value() && Count != N) {
      Count += 1;
    }
  }

  return std::nullopt;
}

template <typename T>
[[nodiscard]] std::optional<syntax::SyntaxToken>
token(syntax::SyntaxNode Node, syntax::SyntaxKind Kind, size_t N = 0) noexcept {
  size_t Count = 0;
  for (const auto &Child : Node.getChildrenWithTokens()) {
    const bool isMatch = Child.isToken() && Child.getKind() == Kind;
    if (isMatch && Count == N) {
      return Child.getToken();
    } else if (isMatch && Count != N) {
      Count += 1;
    }
  }
}
} // namespace yuzu::ast

#endif // YUZU_AST_AST_H
