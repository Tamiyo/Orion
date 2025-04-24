#ifndef LANG_PARSER_GRAMMAR_EXPRESSION_POSTFIX_OP_H_
#define LANG_PARSER_GRAMMAR_EXPRESSION_POSTFIX_OP_H_

#include <cstdint>
#include <optional>
#include <tuple>

namespace yuzu::lang {
inline std::optional<std::tuple<uint8_t, uint8_t>> PostfixBindingPower(
    const std::optional<TokenKind> kind) {
  if (!kind) {
    return std::nullopt;
  }

  // TODO(tamiyo) Implement this.

  switch (kind.value()) {
    default:
      return std::nullopt;
  }
}
}  // namespace yuzu::lang

#endif  // LANG_PARSER_GRAMMAR_EXPRESSION_POSTFIX_OP_H_
