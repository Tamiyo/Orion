#ifndef LANG_PARSER_GRAMMAR_EXPRESSION_INFIX_OP_H_
#define LANG_PARSER_GRAMMAR_EXPRESSION_INFIX_OP_H_

#include <cstdint>
#include <optional>
#include <tuple>

#include "lang/lexer/token_kind.h"

namespace yuzu::lang {
inline std::optional<std::tuple<uint8_t, uint8_t>> InfixBindingPower(
    const std::optional<TokenKind> kind) {
  if (!kind) {
    return std::nullopt;
  }

  switch (kind.value()) {
    case TokenKind::kPlus:
    case TokenKind::kMinus:
      return std::make_tuple(1, 2);
    case TokenKind::kAsterisk:
    case TokenKind::kSlash:
    case TokenKind::kPercent:
      return std::make_tuple(3, 4);
    default:
      return std::nullopt;
  }
}
}  // namespace yuzu::lang

#endif  // LANG_PARSER_GRAMMAR_EXPRESSION_INFIX_OP_H_
