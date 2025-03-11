#ifndef ORION_SYNTAX_LEXER_ABSTRACT_LEXER_H_
#define ORION_SYNTAX_LEXER_ABSTRACT_LEXER_H_

#include <functional>
#include <optional>
#include <string>
#include <utility>

#include "syntax/lexer/token.h"

// https://en.cppreference.com/w/cpp/string/multibyte
namespace orion::syntax {
class AbstractLexer {
 public:
  virtual std::optional<Token> TryNextToken() = 0;

 protected:
  explicit AbstractLexer(std::string source,
                         const bool is_case_sensitive = false)
      : source_(std::move(source)), is_case_sensitive_(is_case_sensitive) {
    source_length_ = source_.length();
    start_ = 0;
    end_ = 0;
  }

  ~AbstractLexer() = default;

  // Utils
  template <typename TokenKind = uint16_t>
  Token CreateToken(TokenKind kind) {
    const size_t distance = end_ - start_;
    const std::string source = source_.substr(start_, distance);
    const auto span = Span(start_, end_);
    const auto token = Token(static_cast<uint16_t>(kind), span, source);

    start_ = end_;
    return token;
  }

  template <typename TokenKind = uint16_t>
  Token ConsumeAndCreateToken(TokenKind kind, const size_t count = 1) {
    Consume(count);
    return CreateToken<TokenKind>(kind);
  }

  // State Management
  [[nodiscard]] bool AtEnd(size_t offset = 0) const;

  // Peek
  [[nodiscard]] char GetCurrent() const;

  // Check
  [[nodiscard]] bool IsCurrent(char ch, size_t offset = 0) const;
  [[nodiscard]] bool IsCurrent(const std::function<int(int)>& predicate,
                               size_t offset = 0) const;
  [[nodiscard]] bool IsCurrent2(char ch1, char ch2, size_t offset = 0) const;
  [[nodiscard]] bool IsCurrent3(char ch1, char ch2, char ch3,
                                size_t offset = 0) const;
  [[nodiscard]] bool IsCurrentSubstr(const std::string& substr) const;

  // Consume
  void Consume(size_t count = 1);
  void ConsumeIf(bool condition);
  void ConsumeWhile(const std::function<bool(char)>& predicate);
  void TryConsume(char ch);
  void TryConsume2(char ch1, char ch2);

 private:
  std::string source_;
  size_t source_length_;
  size_t start_;
  size_t end_;
  bool is_case_sensitive_;
};
}  // namespace orion::syntax
#endif  // ORION_SYNTAX_LEXER_ABSTRACT_LEXER_H_