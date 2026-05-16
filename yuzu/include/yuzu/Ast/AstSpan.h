#ifndef YUZU_AST_AST_SPAN_H
#define YUZU_AST_AST_SPAN_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Lexer/Range.h"

#include <cstdint>
#include <optional>

namespace yuzu::ast {

namespace detail {
/// Find the end offset of the last non-trivia token under `n`. Returns
/// `std::nullopt` if `n` has no non-trivia descendants.
inline std::optional<uint32_t> findLastNonTriviaEnd(const SyntaxNode &n) {
  auto cur = n.getLastChildOrToken();
  while (cur.has_value()) {
    if (cur->isToken()) {
      if (!isTrivia(cur->getKind())) {
        const auto &token = cur->getToken();
        return static_cast<uint32_t>(token.getOffset() +
                                     token.getGreen().getWidth());
      }
    } else if (auto end = findLastNonTriviaEnd(cur->getNode())) {
      return end;
    }
    cur = cur->getPrevSiblingOrToken();
  }
  return std::nullopt;
}
} // namespace detail

/// Range of `node` with trailing trivia trimmed off. The raw
/// `AstNode::getRange()` includes whitespace/newlines owned by the
/// node's last descendant — used directly, diagnostic underlines
/// overshoot the content they point at. Callers that need the raw range
/// can still use `node.getRange()`.
inline lexer::Range tightRange(const AstNode &node) {
  const auto &syntax = node.getSyntax();
  const auto start = static_cast<uint32_t>(syntax.getOffset());
  if (const auto end = detail::findLastNonTriviaEnd(syntax)) {
    return lexer::Range{start, *end};
  }
  return node.getRange();
}

} // namespace yuzu::ast

#endif // YUZU_AST_AST_SPAN_H
