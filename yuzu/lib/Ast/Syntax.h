#ifndef YUZU_AST_SYNTAX_KIND_H
#define YUZU_AST_SYNTAX_KIND_H

#include <cstdint>

namespace yuzu::ast {
enum class SyntaxKind : uint16_t {
  // Tokens
  Plus,

  // Nodes
  InfixExpr,
  LiteralExpr,
  ParenExpr,

  // Literals
  Number
};
} // namespace yuzu::ast

#endif // YUZU_AST_SYNTAX_KIND_H
