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
  [[nodiscard]] std::optional<lexer::Token> getNextToken() {
    bumpTrivia();

    if (cursor >= tokens.size()) {
      return std::nullopt;
    }

    const auto token = tokens.at(cursor);
    cursor += 1;
    return token;
  }

  /// \brief Peek at the final token in the source without consuming it.
  ///
  /// Returns the last token regardless of trivia status; does not advance
  /// the cursor.
  ///
  /// \return The last Token in the input, or nullopt if the source is empty.
  [[nodiscard]] std::optional<lexer::Token> peekLastToken() {
    if (tokens.empty()) {
      return std::nullopt;
    }
    return tokens.back();
  }

  /// \brief Peek at the next non-trivia token without consuming it.
  ///
  /// Automatically skips over any trivia tokens before peeking at the next
  /// meaningful token.
  ///
  /// \return The next non-trivia Token, or nullopt if at end of input.
  [[nodiscard]] std::optional<lexer::Token> peekNextToken() {
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
  [[nodiscard]] std::optional<lexer::TokenKind> peekNextKind() {
    bumpTrivia();

    if (cursor >= tokens.size()) {
      return std::nullopt;
    }

    const auto token = tokens.at(cursor);
    return token.getKind();
  }

  /// \brief Peek at the kind of the nth-following non-trivia token.
  ///
  /// `offset` 0 is the next non-trivia token (same as `peekNextKind`), 1 is
  /// the one after it, and so on. Trivia is skipped without advancing the
  /// cursor.
  ///
  /// \return The TokenKind at that offset, or nullopt if it runs off the end.
  [[nodiscard]] std::optional<lexer::TokenKind> peekKindAhead(size_t offset) {
    size_t remaining = offset;
    for (size_t scan = cursor; scan < tokens.size(); scan += 1) {
      if (lexer::isTrivia(tokens.at(scan).getKind())) {
        continue;
      }
      if (remaining == 0) {
        return tokens.at(scan).getKind();
      }
      remaining -= 1;
    }
    return std::nullopt;
  }

private:
  /// \brief Skip over the next token if it is trivia.
  void bumpTrivia() {
    while (cursor < tokens.size() && atTrivia()) {
      cursor += 1;
    }
  }

  /// \brief Check if the next token is trivia.
  ///
  /// \return true if the next token is trivia, false otherwise.
  bool atTrivia() {
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
