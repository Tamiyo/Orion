#ifndef AST_EXPR_H
#define AST_EXPR_H

#include "Syntax/Syntax.h"
#include "Syntax/SyntaxIterator.h"
#include "Util/ErrorHandling.h"

#include <cassert>
#include <memory>
#include <optional>
#include <variant>

namespace yuzu::ast {

enum class ExprKind { BinaryExpr, ParenExpr, LiteralExpr };

class Expr {
public:
  [[nodiscard]] static std::optional<Expr>
  tryFrom(const syntax::SyntaxNode &Node);

  [[nodiscard]] template <typename T> std::optional<T> tryCast() const {
    T Result(Node_);

    if (this->getKind() == Result.getKind()) {
      return Result;
    }

    return std::nullopt;
  }

  [[nodiscard]] bool is(ExprKind Kind) const noexcept {
    return Kind == getKind();
  }

  [[nodiscard]] ExprKind getKind() const noexcept { return Kind_; }

protected:
  explicit Expr(const syntax::SyntaxNode &Node, ExprKind Kind)
      : Node_(Node), Kind_(Kind) {}

  const syntax::SyntaxNode Node_;
  const ExprKind Kind_;
};

class BinaryExpr : public Expr {
public:
  explicit BinaryExpr(const syntax::SyntaxNode &Node)
      : Expr(Node, ExprKind::BinaryExpr) {}

  BinaryExpr() = delete;

  [[nodiscard]] std::optional<Expr> getLhs() const {
    const syntax::SyntaxChildren Children = Node_.getChildren();

    for (auto It = Children.begin(), End = Children.end(); It != End; It++) {
      const syntax::SyntaxNode Node = *It;

      if (const std::optional<Expr> CastNode = Expr::tryFrom(Node)) {
        return CastNode;
      }
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<Expr> getRhs() const {
    const syntax::SyntaxChildren Children = Node_.getChildren();

    bool LookingForSecondExpr = false;
    for (auto It = Children.begin(), End = Children.end(); It != End; It++) {
      const syntax::SyntaxNode Node = *It;

      if (const std::optional<Expr> CastNode = Expr::tryFrom(Node)) {
        if (LookingForSecondExpr) {
          return CastNode;
        }

        LookingForSecondExpr = true;
      }
    }

    return std::nullopt;
  }
};

class ParenExpr : public Expr {
public:
  explicit ParenExpr(const syntax::SyntaxNode &Node)
      : Expr(Node, ExprKind::ParenExpr) {}

  ParenExpr() = delete;

  [[nodiscard]] std::optional<Expr> getValue() {
    const syntax::SyntaxChildren Children = Node_.getChildren();

    for (auto It = Children.begin(), End = Children.end(); It != End; It++) {
      const syntax::SyntaxNode Node = *It;

      if (const std::optional<Expr> CastNode = Expr::tryFrom(Node)) {
        return CastNode;
      }
    }

    return std::nullopt;
  }
};

class LiteralExpr : public Expr {
public:
  explicit LiteralExpr(const syntax::SyntaxNode &Node)
      : Expr(Node, ExprKind::LiteralExpr) {}

  [[nodiscard]] std::u32string_view getValue() const {
    const syntax::SyntaxChildrenWithTokens ChildrenWithTokens =
        Node_.getChildrenWithTokens();

    for (auto It = ChildrenWithTokens.begin(), End = ChildrenWithTokens.end();
         It != End; It++) {

      const syntax::SyntaxElement Element = *It;
      if (auto Token = std::get_if<syntax::SyntaxToken>(&Element)) {
        return Token->getGreen().getSource();
      }
    }

    util::yuzu_unreachable();
  }

  LiteralExpr() = delete;
};
} // namespace yuzu::ast

#endif // AST_EXPR_H
