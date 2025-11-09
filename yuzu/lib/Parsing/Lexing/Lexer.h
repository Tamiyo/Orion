#ifndef YUZU_PARSING_LEXING_LEXER_H
#define YUZU_PARSING_LEXING_LEXER_H

#include <string_view>

namespace yuzu::parsing {
class Lexer final {
  explicit Lexer(const std::u32string_view &source)
      : source(std::move(source)) {}

private:
  const std::u32string_view source;
};

} // namespace yuzu::parsing

#endif // YUZU_PARSING_LEXING_LEXER_H
