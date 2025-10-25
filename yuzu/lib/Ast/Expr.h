#ifndef YUZU_AST_EXPR_H
#define YUZU_AST_EXPR_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Ast/ExprBuilder.h"
#include "yuzu/Syntax/Syntax.h"
#include "yuzu/Syntax/SyntaxIterator.h"
#include "yuzu/Util/ErrorHandling.h"

#include <memory>
#include <variant>

namespace yuzu::ast {
class Expr;

class BinaryExpr final : public AstNode {
public:
  explicit BinaryExpr(syntax::SyntaxNode Node) : AstNode(std::move(Node)) {}

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

class ParenExpr final : public AstNode {
public:
  explicit ParenExpr(syntax::SyntaxNode Node) : AstNode(std::move(Node)) {}

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

class LiteralExpr final : public AstNode {
public:
  explicit LiteralExpr(syntax::SyntaxNode Node) : AstNode(std::move(Node)) {}

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

class Expr : public AstNodeLike,
             public std::variant<BinaryExpr, ParenExpr, LiteralExpr> {
  using std::variant<BinaryExpr, ParenExpr, LiteralExpr>::variant;

  Expr() = delete;
};
} // namespace yuzu::ast

#endif // YUZU_AST_EXPR_H
