#ifndef AST_SYNTAX_KIND_H
#define AST_SYNTAX_KIND_H

#include "Syntax/SyntaxKind.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <variant>

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

#endif // AST_SYNTAX_KIND_H
