#ifndef LANG_PARSER_GRAMMAR_EXPRESSION_POSTFIX_OP_H_
#define LANG_PARSER_GRAMMAR_EXPRESSION_POSTFIX_OP_H_

#include <cstdint>
#include <tuple>

namespace yuzu::lang {
enum class PostfixOp : uint16_t {
  // TODO(tamiyo) Add Postfix operators
};

std::tuple<uint8_t, uint8_t> BindingPower(const PostfixOp op) {
  switch (op) {
    default:
      return std::make_tuple(0, 0);
  }
}

// TODO(tamiyo) Implement this.
std::optional<PostfixOp> PostfixOpFromTokenKind(const TokenKind kind) {
  return std::nullopt;
}
}  // namespace yuzu::lang

#endif  // LANG_PARSER_GRAMMAR_EXPRESSION_POSTFIX_OP_H_
