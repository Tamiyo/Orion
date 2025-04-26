#ifndef SYNTAX_PARSER_TOKEN_SOURCE_H_
#define SYNTAX_PARSER_TOKEN_SOURCE_H_

#include <cstddef>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "syntax/lexer/token.h"

namespace yuzu::syntax {
/// \brief Provides a source for retrieving tokens during parsing.
///
/// The `TokenSource` class manages a sequence of tokens and provides methods
/// to retrieve the next token or peek at upcoming tokens while handling
/// trivia (whitespace, comments) automatically.
template <typename TokenKind = uint16_t>
class TokenSource {
 private:
  using Token = Token<TokenKind>;

 public:
  /// \brief Constructs a `TokenSource` with a given vector of tokens.
  ///
  /// \param tokens The tokens to be managed by this source.
  /// \param is_trivia Determines if a TokenKind is a trivia token.
  explicit TokenSource(const std::vector<Token>& tokens,
                       const std::function<bool(TokenKind)>& is_trivia)
      : tokens_(std::move(tokens)), is_trivia_(is_trivia), token_idx_(0) {}

  /// \brief Deleted default constructor.
  ///
  /// A `TokenSource` must always be initialized with a vector of tokens.
  TokenSource() = delete;

  /// \brief Retrieves the next token from the source, skipping any trivia.
  ///
  /// \return An optional containing the next token if available, otherwise
  /// `std::nullopt`.
  [[nodiscard]] std::optional<Token> NextToken() noexcept {
    BumpTrivia();

    if (token_idx_ < tokens_.size()) {
      const Token token = tokens_.at(token_idx_);
      token_idx_ += 1;
      return token;
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<Span> LastTokenSpan() noexcept {
    if (!tokens_.empty()) {
      return tokens_.back().Span();
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
    if (const auto kind = PeekKindRaw(); kind.has_value()) {
      return is_trivia_(*kind);
    }

    return false;
  }

  /// \brief Peeks at the kind of the next token without consuming it.
  ///
  /// \return An optional containing the kind of the next token if available,
  /// otherwise `nullopt`.
  [[nodiscard]] std::optional<TokenKind> PeekKindRaw() const noexcept {
    if (const auto token = PeekTokenRaw(); token.has_value()) {
      return token->Kind();
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
  const std::vector<Token> tokens_;
  const std::function<bool(TokenKind)>& is_trivia_;
  size_t token_idx_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_TOKEN_SOURCE_H_
