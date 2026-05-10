#include "yuzu/Ast/Ast.h"

#include <optional>

namespace yuzu::ast {
std::optional<BinOp> BinaryExpr::getOp() const {
  for (const auto &child : node.getChildrenWithTokens()) {
    if (child.isToken()) {
      const auto &token = child.getToken();
      switch (token.getKind()) {
      case SyntaxKind::Minus:
        return BinOp::Sub;
      case SyntaxKind::Plus:
        return BinOp::Add;
      case SyntaxKind::Slash:
        return BinOp::Div;
      case SyntaxKind::Star:
        return BinOp::Mul;
      default:
        break;
      }

      break;
    }
  }

  return std::nullopt;
}

} // namespace yuzu::ast
