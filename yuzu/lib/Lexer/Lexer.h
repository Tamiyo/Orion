#ifndef YUZU_LEXER_LEXER_H
#define YUZU_LEXER_LEXER_H

#include "yuzu/Lexer/Token.h"

#include <optional>
#include <string_view>

namespace yuzu::lexer {
/// \brief Lexical analyzer for tokenizing source code.
///
/// Lexer processes a UTF-32 encoded source text and converts it into a
/// stream of tokens. It maintains internal state to track the current
/// position in the source text and provides methods to retrieve tokens
/// sequentially.
class Lexer final {
  /// \brief Construct a lexer for the given source text.
  ///
  /// \param source A view of the UTF-32 encoded source text to tokenize.
  explicit Lexer(const std::u32string_view &source)
      : current(source.data()), end(source.data() + source.length()) {}

  /// \brief Get the next token from the source text.
  ///
  /// \return The next Token in the source text.
  [[nodiscard]] Token getNextToken() noexcept;

private:
  /// \brief Peek at the current character without consuming it.
  ///
  /// \return The current character, or nullopt if at end of input.
  [[nodiscard]] std::optional<char32_t> peek() const noexcept {
    if (current >= end) {
      return std::nullopt;
    }

    return *current;
  }

  /// Pointer to the current position in the source text.
  const char32_t *current;

  /// Pointer to one past the end of the source text.
  const char32_t *end;
};

} // namespace yuzu::lexer

#endif // YUZU_LEXER_LEXER_H
