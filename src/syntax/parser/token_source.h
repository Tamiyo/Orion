#ifndef SYNTAX_PARSER_TOKEN_SOURCE_H_
#define SYNTAX_PARSER_TOKEN_SOURCE_H_

#include <optional>
#include <utility>
#include <vector>

#include "syntax/lexer/token.h"
#include "syntax/lexer/token_kind.h"

namespace orion::syntax {
class TokenSource {
 public:
  explicit TokenSource(std::vector<Token> tokens)
      : tokens_(std::move(tokens)), token_idx_(0) {}

  TokenSource() = delete;

  [[nodiscard]] std::optional<Token> NextToken() noexcept {
    ConsumeTrivia();

    if (token_idx_ < tokens_.size()) {
      const Token token = tokens_.at(token_idx_);
      token_idx_ += 1;
      return token;
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<TokenKind> PeekKind() noexcept {
    ConsumeTrivia();
    return PeekKindRaw();
  }

  [[nodiscard]] std::optional<Token> PeekToken() noexcept {
    ConsumeTrivia();
    return PeekTokenRaw();
  }

 private:
  void ConsumeTrivia() noexcept {
    while (AtTrivia()) {
      token_idx_ += 1;
    }
  }

  [[nodiscard]] bool AtTrivia() const noexcept {
    if (const std::optional<TokenKind> kind = PeekKindRaw(); kind.has_value()) {
      return IsTrivia(*kind);
    }

    return false;
  }

  [[nodiscard]] std::optional<TokenKind> PeekKindRaw() const noexcept {
    if (const std::optional<Token> token = PeekTokenRaw(); token.has_value()) {
      return token->Kind<TokenKind>();
    }

    return std::nullopt;
  }

  [[nodiscard]] std::optional<Token> PeekTokenRaw() const noexcept {
    if (token_idx_ < tokens_.size()) {
      return tokens_.at(token_idx_);
    }

    return std::nullopt;
  }

  const std::vector<Token> tokens_;
  size_t token_idx_;
};
}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_TOKEN_SOURCE_H_
