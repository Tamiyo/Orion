#include "yuzu/Ast/Ast.h"

#include <cstdint>
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

std::optional<Int64> LiteralExpr::getValue() const {
  const auto numberToken = token(node, SyntaxKind::Number);
  if (!numberToken) {
    return std::nullopt;
  }

  int64_t value = 0;
  for (const char32_t c : numberToken->getGreen().getSource()) {
    if (c < U'0' || c > U'9') {
      return std::nullopt;
    }
    value = value * 10 + static_cast<int64_t>(c - U'0');
  }
  return value;
}

} // namespace yuzu::ast
