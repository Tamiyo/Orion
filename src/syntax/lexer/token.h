#ifndef ORION_SYNTAX_LEXER_TOKEN_H_
#define ORION_SYNTAX_LEXER_TOKEN_H_

#include <cstdint>
#include <string>
#include <utility>

#include "syntax/lexer/span.h"

namespace orion::syntax {
class Token {
 public:
  Token(const uint16_t kind, const Span span, std::string source)
      : kind_(kind), span_(span), source_(std::move(source)) {}

  template <typename TokenKind>
  TokenKind GetKind() const {
    return static_cast<TokenKind>(kind_);
  }

  [[nodiscard]] Span GetSpan() const { return span_; }
  [[nodiscard]] std::string GetSource() const { return source_; }

  bool operator==(const Token &other) const {
    return kind_ == other.kind_ && source_ == other.source_ &&
           span_ == other.span_;
  }

 private:
  uint16_t kind_;
  Span span_;
  std::string source_;
};
}  // namespace orion::syntax
#endif  // ORION_SYNTAX_LEXER_TOKEN_H_