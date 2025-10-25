#ifndef YUZU_AST_AST_H
#define YUZU_AST_AST_H

#include "yuzu/Syntax/Syntax.h"

namespace yuzu::ast {
class AstNodeLike {
public:
  virtual ~AstNodeLike() = default;

  template <typename Subtype>[[nodiscard]] bool is() const noexcept {
    return dynamic_cast<const Subtype *>(this) != nullptr;
  }

  template <typename Subtype>
  [[nodiscard]] std::optional<const Subtype *const> tryAs() const {
    if (is<Subtype>()) {
      return static_cast<const Subtype *>(this);
    }

    return std::nullopt;
  }
};

class AstNode : public AstNodeLike {
public:
  virtual ~AstNode() = default;

protected:
  explicit AstNode(syntax::SyntaxNode Node) : Node_(std::move(Node)) {}

  syntax::SyntaxNode Node_;
};
} // namespace yuzu::ast

#endif // YUZU_AST_AST_H
