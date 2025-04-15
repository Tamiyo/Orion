#ifndef LANG_PARSER_GRAMMAR_EXPRESSION_PREFIX_OP_H_
#define LANG_PARSER_GRAMMAR_EXPRESSION_PREFIX_OP_H_

#include <cstdint>
#include <tuple>

namespace yuzu::lang {
enum class PrefixOp : uint16_t {
  // TODO(tamiyo) Add Postfix operators
};

std::tuple<uint8_t, uint8_t> BindingPower(const PrefixOp op) {
  switch (op) {
    default:
      return std::make_tuple(0, 0);
  }
}

// TODO(tamiyo) Implement this.
std::optional<PrefixOp> PrefixOpFromTokenKind(const TokenKind kind) {
  return std::nullopt;
}
}  // namespace yuzu::lang

#endif  // LANG_PARSER_GRAMMAR_EXPRESSION_PREFIX_OP_H_
