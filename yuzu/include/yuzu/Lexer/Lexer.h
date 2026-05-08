#ifndef YUZU_LEXER_LEXER_H
#define YUZU_LEXER_LEXER_H

#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include <cassert>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace yuzu::lexer {
/// \brief Lexical analyzer for tokenizing source code.
///
/// Lexer processes a UTF-32 encoded source text and converts it into a
/// stream of tokens. It maintains internal state to track the current
/// position in the source text and provides methods to retrieve tokens
/// sequentially.
class [[nodiscard]] Lexer {
public:
  /// \brief Construct a lexer for the given source text.
  ///
  /// \param source A view of the UTF-32 encoded source text to tokenize.
  explicit Lexer(std::u32string_view source)
      : source(std::move(source)), current(source.data()) {}

  /// \brief Tokenize the entire source text.
  ///
  /// Processes the source from the current position to the end, collecting
  /// all tokens into a vector. This consumes the entire input.
  ///
  /// \return A vector containing all tokens from the source text.
  [[nodiscard]] std::vector<Token> getTokens() {
    auto tokens = std::vector<Token>();
    while (true) {
      const auto token = getNextToken();
      if (!token.has_value()) {
        break;
      }

      tokens.emplace_back(token.value());
    }

    return tokens;
  }

  /// \brief Reset the lexer to the start of the source text.
  ///
  /// Resets the current position to the beginning and clears any cached
  /// peeked token.
  void reset() { current = source.begin(); }

private:
  /// \brief Check if the current character is a digit.
  ///
  /// \return true if current is within bounds and is a digit ('0'-'9').
  [[nodiscard]] static bool atDigit(const char32_t *ch) {
    return *ch >= U'0' && *ch <= U'9';
  }

  /// \brief Check if the current character is whitespace.
  ///
  /// \return true if current is within bounds and is whitespace.
  [[nodiscard]] static bool atWhitespace(const char32_t *ch) {
    return (*ch == U' ' || *ch == U'\t' || *ch == U'\n' || *ch == U'\r');
  }

  /// \brief Check if the current character is alphabetic.
  ///
  /// \return true if current is within bounds and is a letter (a-z or A-Z).
  [[nodiscard]] static bool atAlpha(const char32_t *ch) {
    return ((*ch >= U'a' && *ch <= U'z') || (*ch >= U'A' && *ch <= U'Z'));
  }

  /// \brief Check if the current character is a valid identifier character.
  ///
  /// \return true if current is a letter, digit, or underscore.
  [[nodiscard]] static bool atIdent(const char32_t *ch) {
    return ((*ch >= U'a' && *ch <= U'z') || (*ch >= U'A' && *ch <= U'Z') ||
            (*ch >= U'0' && *ch <= U'9') || (*ch == U'_'));
  }

  /// \brief Check if the current character is a valid identifier starting
  /// character.
  ///
  /// \return true if current is a letter, digit, or underscore.
  [[nodiscard]] static bool atIdentStart(const char32_t *ch) {
    return ((*ch >= U'a' && *ch <= U'z') || (*ch >= U'A' && *ch <= U'Z') ||
            (*ch == U'_'));
  }

  /// \brief Get the next token from the source text.
  ///
  /// \return The next Token in the source text.
  [[nodiscard]] std::optional<Token> getNextToken();

  /// \brief Advance the current position by n characters.
  ///
  /// \param n The number of characters to advance (default 1).
  void bump(const size_t n = 1) {
    if (current >= source.end()) {
      return;
    }

    size_t i = 0;
    while (current < source.end() && i < n) {
      current += 1;
      i += 1;
    }
  }

  /// \brief Advance the current position while the predicate is true.
  ///
  /// \param predicate A function that returns true for characters to skip.
  void bumpWhile(std::function<bool(const char32_t *)> predicate) {
    while (current < source.end() && predicate(current)) {
      bump();
    }
  }

  /// \brief Create a token from the start position to the current position.
  ///
  /// \param start Pointer to the beginning of the token.
  /// \param kind The kind of token to create.
  /// \return A Token spanning from start to current position.
  Token createToken(const char32_t *start, TokenKind kind) const {
    const ptrdiff_t startIndex = (start - source.begin());
    const ptrdiff_t endIndex = (current - source.begin());

    assert(startIndex >= 0 && "Token start cannot be negative");
    assert(endIndex >= 0 && "Token end cannot be negative");

    assert(startIndex <= std::numeric_limits<uint32_t>::max() &&
           "Token start exceeds uint32_t range");
    assert(endIndex <= std::numeric_limits<uint32_t>::max() &&
           "Token end exceeds uint32_t range");

    const auto source = std::u32string_view(start, current - start);
    const auto range = Range{
        .start = static_cast<uint32_t>(startIndex),
        .end = static_cast<uint32_t>(endIndex),
    };

    return Token(kind, source, range);
  }

  /// \brief Peek at the current character without consuming it.
  ///
  /// \return The current character, or nullopt if at end of input.
  [[nodiscard]] std::optional<char32_t> peek(const size_t n = 0) const {
    if ((current + n) >= source.end()) {
      return std::nullopt;
    }

    return *(current + n);
  }

  /// \brief The UTF-32 encoded source text being tokenized.
  const std::u32string_view source;

  /// \brief Pointer to the current position in the source text.
  const char32_t *current;
};

} // namespace yuzu::lexer

#endif // YUZU_LEXER_LEXER_H
