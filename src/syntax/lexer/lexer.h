#ifndef SYNTAX_LEXER_LEXER_H_
#define SYNTAX_LEXER_LEXER_H_

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "syntax/lexer/token.h"

namespace orion::syntax {
class Lexer {
 public:
  Lexer() = delete;
  virtual ~Lexer() = default;

  std::vector<Token> Tokenize() noexcept;

 protected:
  explicit Lexer(std::u32string source)
      : source_(std::move(source)),
        source_length_(source_.length()),
        start_(0),
        end_(0) {}

  virtual std::optional<Token> TryNextToken() noexcept = 0;

  // Utils
  template <typename TokenKind = uint16_t>
  [[nodiscard]] Token CreateToken(TokenKind kind) noexcept {
    const size_t distance = end_ - start_;
    const std::u32string source = source_.substr(start_, distance);
    const auto span = Span(start_, end_);
    const auto token = Token(static_cast<uint16_t>(kind), span, source);

    start_ = end_;
    return token;
  }

  template <typename TokenKind = uint16_t>
  Token ConsumeAndCreateToken(TokenKind kind, const size_t count = 1) noexcept {
    Consume(count);
    return CreateToken<TokenKind>(kind);
  }

  // State Management
  [[nodiscard]] bool AtEnd(size_t offset = 0) const noexcept {
    return end_ + offset >= source_length_;
  }

  // Peek
  [[nodiscard]] char32_t GetCurrent() const { return source_.at(end_); }

  // Check
  [[nodiscard]] bool At(char32_t ch, size_t offset = 0) const;
  [[nodiscard]] bool At(const std::u32string& value, size_t offset = 0) const;
  [[nodiscard]] bool At(const std::function<bool(char32_t)>& predicate,
                        size_t offset = 0) const;
  [[nodiscard]] bool At2(char32_t ch1, char32_t ch2, size_t offset = 0) const;
  [[nodiscard]] bool At3(char32_t ch1, char32_t ch2, char32_t ch3,
                         size_t offset = 0) const;

  // Consume
  void Consume(size_t count = 1);
  void ConsumeIf(bool condition);
  void ConsumeWhile(const std::function<bool(char32_t)>& predicate);
  void TryConsume(char32_t ch);
  void TryConsume2(char32_t ch1, char32_t ch2);

 private:
  const std::u32string source_;
  const size_t source_length_;
  size_t start_;
  size_t end_;
};
}  // namespace orion::syntax
#endif  // SYNTAX_LEXER_LEXER_H_
