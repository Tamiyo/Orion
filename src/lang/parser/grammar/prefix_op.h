#ifndef LANG_PARSER_GRAMMAR_PREFIX_OP_H_
#define LANG_PARSER_GRAMMAR_PREFIX_OP_H_

#include <cstdint>
#include <optional>
#include <tuple>

namespace yuzu::lang {
inline std::optional<std::tuple<uint8_t, uint8_t>> PrefixBindingPower(
    const std::optional<TokenKind> kind) {
  if (!kind) {
    return std::nullopt;
  }

  // TODO(tamiyo) Implement this.

  switch (kind.value()) {
    case TokenKind::kPlus:
    case TokenKind::kMinus:
      return std::make_tuple(1, 2);

    default:
      return std::nullopt;
  }
}
}  // namespace yuzu::lang

#endif  // LANG_PARSER_GRAMMAR_PREFIX_OP_H_
