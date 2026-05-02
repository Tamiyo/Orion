#ifndef YUZU_PARSER_TOKEN_SOURCE_H
#define YUZU_PARSER_TOKEN_SOURCE_H

#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"

#include <optional>
#include <utility>
#include <vector>

namespace yuzu::parser {
/// \brief Token stream abstraction that automatically filters trivia tokens.
///
/// TokenSource wraps a Lexer and provides a clean interface for parser
/// consumption by automatically skipping over trivia tokens (whitespace,
/// comments, etc.) when retrieving or peeking at tokens.
class [[nodiscard]] TokenSource final {
public:
  /// \brief Construct a token source from the given lexer.
  ///
  /// \param lexer The lexer to use as the underlying token stream.
  explicit TokenSource(const std::vector<lexer::Token> &tokens)
      : tokens(std::move(tokens)), cursor(0) {};

  TokenSource() = delete;

  /// \brief Get the next non-trivia token from the source.
  ///
  /// Automatically skips over any trivia tokens before returning the next
  /// meaningful token.
  ///
  /// \return The next non-trivia Token, or nullopt if at end of input.
  [[nodiscard]] std::optional<lexer::Token> getNextToken() noexcept {
    bumpTrivia();

    if (cursor >= tokens.size()) {
      return std::nullopt;
    }

    const auto token = tokens.at(cursor);
    cursor += 1;
    return token;
  }

  [[nodiscard]] std::optional<lexer::Token> peekLastToken() noexcept {
    if (tokens.empty()) {
      return std::nullopt;
    }
    return *tokens.end();
  }

  /// \brief Peek at the next non-trivia token without consuming it.
  ///
  /// Automatically skips over any trivia tokens before peeking at the next
  /// meaningful token.
  ///
  /// \return The next non-trivia Token, or nullopt if at end of input.
  [[nodiscard]] std::optional<lexer::Token> peekNextToken() noexcept {
    bumpTrivia();

    if (cursor >= tokens.size()) {
      return std::nullopt;
    }

    const auto token = tokens.at(cursor);
    return token;
  }

  /// \brief Peek at the kind of the next non-trivia token.
  ///
  /// Automatically skips over any trivia tokens before peeking at the next
  /// meaningful token kind.
  ///
  /// \return The TokenKind of the next non-trivia token, or nullopt if at end.
  [[nodiscard]] std::optional<lexer::TokenKind> peekNextKind() noexcept {
    bumpTrivia();

    if (cursor >= tokens.size()) {
      return std::nullopt;
    }

    const auto token = tokens.at(cursor);
    return token.getKind();
  }

private:
  /// \brief Skip over the next token if it is trivia.
  void bumpTrivia() noexcept {
    while (cursor < tokens.size() && atTrivia()) {
      cursor += 1;
    }
  }

  /// \brief Check if the next token is trivia.
  ///
  /// \return true if the next token is trivia, false otherwise.
  bool atTrivia() noexcept {
    if (cursor >= tokens.size()) {
      return false;
    }

    const auto token = tokens.at(cursor);
    return lexer::isTrivia(token.getKind());
  }

  /// Underlying lexer providing the token stream.
  const std::vector<lexer::Token> tokens;
  size_t cursor;
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_TOKEN_SOURCE_H
