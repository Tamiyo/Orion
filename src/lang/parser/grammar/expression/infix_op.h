#ifndef LANG_PARSER_GRAMMAR_EXPRESSION_INFIX_OP_H_
#define LANG_PARSER_GRAMMAR_EXPRESSION_INFIX_OP_H_

#include <cstdint>
#include <optional>
#include <tuple>

#include "lang/lexer/token_kind.h"

namespace yuzu::lang {
enum class InfixOp : uint16_t {
  // TODO(tamiyo) Add Infix operators
  kAdd,
  kSub,
  kMul,
  kDiv,
  kMod,
};

std::tuple<uint8_t, uint8_t> BindingPower(const InfixOp op) {
  switch (op) {
    case InfixOp::kAdd:
    case InfixOp::kSub:
      return std::make_tuple(1, 2);
    case InfixOp::kMul:
    case InfixOp::kDiv:
    case InfixOp::kMod:
      return std::make_tuple(3, 4);
    default:
      return std::make_tuple(0, 0);
  }
}

// TODO(tamiyo) Implement this.
std::optional<InfixOp> InfixOpFromTokenKind(const TokenKind kind) {
  return std::nullopt;
}
}  // namespace yuzu::lang

#endif  // LANG_PARSER_GRAMMAR_EXPRESSION_INFIX_OP_H_
