#ifndef SYNTAX_PARSER_TOKEN_SOURCE_H_
#define SYNTAX_PARSER_TOKEN_SOURCE_H_

#include <optional>
#include <utility>
#include <vector>

#include "syntax/lexer/token.h"
#include "syntax/lexer/token_kind.h"

namespace orion::syntax {
/// \brief Provides a source for retrieving tokens during parsing.
///
/// The `TokenSource` class manages a sequence of tokens and provides methods
/// to retrieve the next token or peek at upcoming tokens while handling
/// trivia (whitespace, comments) automatically.
class TokenSource {
 public:
  /// \brief Constructs a `TokenSource` with a given vector of tokens.
  ///
  /// \param tokens The tokens to be managed by this source.
  explicit TokenSource(std::vector<Token> tokens)
      : tokens_(std::move(tokens)), token_idx_(0) {}

  /// \brief Deleted default constructor.
  ///
  /// A `TokenSource` must always be initialized with a vector of tokens.
  TokenSource() = delete;

  /// \brief Retrieves the next token from the source, skipping any trivia.
  ///
  /// \return An optional containing the next token if available, otherwise
  /// `nullopt`.
  [[nodiscard]] std::optional<Token> NextToken() noexcept {
    BumpTrivia();

    if (token_idx_ < tokens_.size()) {
      const Token token = tokens_.at(token_idx_);
      token_idx_ += 1;
      return token;
    }

    return std::nullopt;
  }

  /// \brief Peeks at the kind of the next token without consuming it.
  ///
  /// \return An optional containing the kind of the next token if available,
  /// otherwise `nullopt`.
  [[nodiscard]] std::optional<TokenKind> PeekKind() noexcept {
    BumpTrivia();
    return PeekKindRaw();
  }

  /// \brief Peeks at the next token without consuming it.
  ///
  /// \return An optional containing the next token if available, otherwise
  /// `nullopt`.
  [[nodiscard]] std::optional<Token> PeekToken() noexcept {
    BumpTrivia();
    return PeekTokenRaw();
  }

 private:
  /// \brief Skips any trivia tokens (e.g., whitespace, comments) in the source.
  void BumpTrivia() noexcept {
    while (AtTrivia()) {
      token_idx_ += 1;
    }
  }

  /// \brief Checks if the current token is a trivia token.
  ///
  /// \return `true` if the current token is trivia, otherwise `false`.
  [[nodiscard]] bool AtTrivia() const noexcept {
    if (const std::optional<TokenKind> kind = PeekKindRaw(); kind.has_value()) {
      return IsTrivia(*kind);
    }

    return false;
  }

  /// \brief Peeks at the kind of the next token without consuming it.
  ///
  /// \return An optional containing the kind of the next token if available,
  /// otherwise `nullopt`.
  [[nodiscard]] std::optional<TokenKind> PeekKindRaw() const noexcept {
    if (const std::optional<Token> token = PeekTokenRaw(); token.has_value()) {
      return token->Kind<TokenKind>();
    }

    return std::nullopt;
  }

  /// \brief Peeks at the next token without consuming it.
  ///
  /// \return An optional containing the next token if available, otherwise
  /// `nullopt`.
  [[nodiscard]] std::optional<Token> PeekTokenRaw() const noexcept {
    if (token_idx_ < tokens_.size()) {
      return tokens_.at(token_idx_);
    }

    return std::nullopt;
  }

  /// The vector of tokens being managed by this source.
  const std::vector<Token> tokens_;

  /// The current index of the token being processed.
  size_t token_idx_;
};

}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_TOKEN_SOURCE_H_
