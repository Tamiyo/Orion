#include "Ast/Expr.h"

#include "Ast/SyntaxKind.h"
#include "Syntax/Syntax.h"

#include <optional>

namespace yuzu::ast {
std::optional<Expr> Expr::tryFrom(const syntax::SyntaxNode &Node) {
  switch (static_cast<SyntaxKind>(Node.getKind())) {
  case SyntaxKind::InfixExpr:
    return BinaryExpr(Node);

  case SyntaxKind::ParenExpr:
    return ParenExpr(Node);

  case SyntaxKind::LiteralExpr:
    return LiteralExpr(Node);

  default:
    break;
  }

  return std::nullopt;
}
} // namespace yuzu::ast
