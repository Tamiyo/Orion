#ifndef YUZU_LEXER_LEXER_H
#define YUZU_LEXER_LEXER_H

#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include <functional>
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
public:
  /// \brief Construct a lexer for the given source text.
  ///
  /// \param source A view of the UTF-32 encoded source text to tokenize.
  explicit Lexer(const std::u32string_view &source)
      : source(std::move(source)), current(source.data()) {}

  /// \brief Get the next token from the source text.
  ///
  /// \return The next Token in the source text.
  [[nodiscard]] std::optional<Token> getNextToken() noexcept;

private:
  /// \brief Check if the current character is a digit.
  ///
  /// \return true if current is within bounds and is a digit ('0'-'9').
  [[nodiscard]] static bool atDigit(const char32_t *ch) noexcept {
    return *ch >= U'0' && *ch <= U'9';
  }

  /// \brief Check if the current character is whitespace.
  ///
  /// \return true if current is within bounds and is whitespace.
  [[nodiscard]] static bool atWhitespace(const char32_t *ch) noexcept {
    return (*ch == U' ' || *ch == U'\t' || *ch == U'\n' || *ch == U'\r');
  }

  /// \brief Check if the current character is alphabetic.
  ///
  /// \return true if current is within bounds and is a letter (a-z or A-Z).
  [[nodiscard]] static bool atAlpha(const char32_t *ch) noexcept {
    return ((*ch >= U'a' && *ch <= U'z') || (*ch >= U'A' && *ch <= U'Z'));
  }

  [[nodiscard]] static bool atIdent(const char32_t *ch) noexcept {
    return ((*ch >= U'a' && *ch <= U'z') || (*ch >= U'A' && *ch <= U'Z') ||
            (*ch >= U'0' && *ch <= U'9') || (*ch == U'_'));
  }

  void bump(const size_t n = 1) noexcept {
    if (current >= source.end()) {
      return;
    }

    size_t i = 0;
    while (current < source.end() && i < n) {
      current += 1;
      i += 1;
    }
  }

  void bumpWhile(std::function<bool(const char32_t *)> predicate) {
    while (current < source.end() && predicate(current)) {
      bump();
    }
  }

  Token createToken(const char32_t *start, TokenKind kind) const noexcept;

  /// \brief Peek at the current character without consuming it.
  ///
  /// \return The current character, or nullopt if at end of input.
  [[nodiscard]] std::optional<char32_t>
  peek(const size_t n = 0) const noexcept {
    if ((current + n) >= source.end()) {
      return std::nullopt;
    }

    return *(current + n);
  }

  /// Pointer to the beginning of the source text.
  const std::u32string_view source;

  /// Pointer to the current position in the source text.
  const char32_t *current;
};

} // namespace yuzu::lexer

#endif // YUZU_LEXER_LEXER_H
