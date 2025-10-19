#ifndef AST_EXPR_H
#define AST_EXPR_H

#include "Ast/ExprBuilder.h"
#include "Syntax/Syntax.h"
#include "Syntax/SyntaxIterator.h"
#include "Util/ErrorHandling.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

namespace yuzu::ast {
class Expr {
public:
  virtual ~Expr() = default;

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

protected:
  explicit Expr(syntax::SyntaxNode Node) : Node_(std::move(Node)) {}

  syntax::SyntaxNode Node_;
};

class BinaryExpr final : public Expr {
public:
  explicit BinaryExpr(syntax::SyntaxNode Node) : Expr(std::move(Node)) {}

  BinaryExpr() = delete;
  BinaryExpr(const BinaryExpr &) = delete;
  BinaryExpr &operator=(const BinaryExpr &) = delete;

  [[nodiscard]] std::unique_ptr<Expr> getLhs() const {
    const syntax::SyntaxChildren Children = Node_.getChildren();

    for (auto It = Children.begin(), End = Children.end(); It != End; It++) {
      syntax::SyntaxNode Node = *It;

      if (std::unique_ptr<Expr> CastNode =
              ExprBuilder::tryFrom(std::move(Node))) {
        return CastNode;
      }
    }

    return nullptr;
  }

  [[nodiscard]] std::unique_ptr<Expr> getRhs() const {
    const syntax::SyntaxChildren Children = Node_.getChildren();

    bool LookingForSecondExpr = false;
    for (auto It = Children.begin(), End = Children.end(); It != End; It++) {
      syntax::SyntaxNode Node = *It;

      if (std::unique_ptr<Expr> CastNode =
              ExprBuilder::tryFrom(std::move(Node))) {
        if (LookingForSecondExpr) {
          return CastNode;
        }

        LookingForSecondExpr = true;
      }
    }

    return nullptr;
  }
};

class ParenExpr final : public Expr {
public:
  explicit ParenExpr(syntax::SyntaxNode Node) : Expr(std::move(Node)) {}

  ParenExpr() = delete;
  ParenExpr(const ParenExpr &) = delete;
  ParenExpr &operator=(const ParenExpr &) = delete;

  [[nodiscard]] std::unique_ptr<Expr> getValue() {
    const syntax::SyntaxChildren Children = Node_.getChildren();

    for (auto It = Children.begin(), End = Children.end(); It != End; It++) {
      syntax::SyntaxNode Node = *It;

      if (std::unique_ptr<Expr> CastNode =
              ExprBuilder::tryFrom(std::move(Node))) {
        return CastNode;
      }
    }

    return nullptr;
  }
};

class LiteralExpr final : public Expr {
public:
  explicit LiteralExpr(syntax::SyntaxNode Node) : Expr(std::move(Node)) {}

  LiteralExpr() = delete;
  LiteralExpr(const LiteralExpr &) = delete;
  LiteralExpr &operator=(const LiteralExpr &) = delete;

  [[nodiscard]] std::u32string_view getValue() const {
    const syntax::SyntaxChildrenWithTokens ChildrenWithTokens =
        Node_.getChildrenWithTokens();

    for (auto It = ChildrenWithTokens.begin(), End = ChildrenWithTokens.end();
         It != End; It++) {

      const auto Element = *It;
      if (const syntax::SyntaxToken *Token = Element.getIfToken()) {
        return Token->getGreen().getSource();
      }
    }

    util::yuzu_unreachable();
  }
};
} // namespace yuzu::ast

#endif // AST_EXPR_H
