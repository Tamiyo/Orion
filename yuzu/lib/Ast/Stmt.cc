#include "yuzu/Ast/Ast.h"

#include <optional>

namespace yuzu::ast {

std::optional<StringView> LetStmt::getName() const {
  for (const auto &e : node.getChildrenWithTokens()) {
    if (!e.isToken()) {
      continue;
    }

    const auto token = e.getToken();
    if (token.getKind() == SyntaxKind::Ident) {
      return token.getGreen().getSource();
    }
  }

  return std::nullopt;
}

} // namespace yuzu::ast
