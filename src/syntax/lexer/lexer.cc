#include "syntax/lexer/lexer.h"

#include <functional>
#include <string>
#include <vector>

namespace orion::syntax {
std::vector<Token> Lexer::Tokenize() noexcept {
  std::vector<Token> tokens = std::vector<Token>();
  while (!AtEnd()) {
    if (const std::optional<Token> token = TryNextToken(); token.has_value()) {
      tokens.emplace_back(token.value());
    } else {
      break;
    }
  }

  return tokens;
}

bool Lexer::At(const char32_t ch, const size_t offset) const {
  if (AtEnd(offset)) {
    return false;
  }

  const size_t current = end_ + offset;
  return source_.at(current) == ch;
}

bool Lexer::At(const std::u32string& value, const size_t offset) const {
  if (AtEnd(offset + value.size() - 1)) {
    return false;
  }

  const std::u32string substring = source_.substr(end_ + offset, value.size());

  return substring == value;
}

bool Lexer::At(const std::function<bool(char32_t)>& predicate,
               const size_t offset) const {
  if (AtEnd(offset)) {
    return false;
  }

  const size_t current = end_ + offset;
  return predicate(source_.at(current));
}

bool Lexer::At2(const char32_t ch1, const char32_t ch2,
                const size_t offset) const {
  return At(ch1, offset) || At(ch2, offset);
}

bool Lexer::At3(const char32_t ch1, const char32_t ch2, const char32_t ch3,
                const size_t offset) const {
  return At(ch1, offset) || At(ch2, offset) || At(ch3, offset);
}

// Consume
void Lexer::Consume(const size_t count) {
  size_t consumed = 0;
  while (!AtEnd() && consumed++ < count) {
    end_ += 1;
  }
}

void Lexer::ConsumeIf(const bool condition) {
  if (!AtEnd() && condition) {
    Consume();
  }
}

void Lexer::ConsumeWhile(const std::function<bool(char32_t)>& predicate) {
  while (!AtEnd() && predicate(source_.at(end_))) {
    end_++;
  }
}

void Lexer::TryConsume(const char32_t ch) {
  if (!AtEnd() && source_.at(end_) == ch) {
    Consume();
  }
}

void Lexer::TryConsume2(const char32_t ch1, const char32_t ch2) {
  if (!AtEnd() && (source_.at(end_) == ch1 || source_.at(end_) == ch2)) {
    Consume();
  }
}
}  // namespace orion::syntax
