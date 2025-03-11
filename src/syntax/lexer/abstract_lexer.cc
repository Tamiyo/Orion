#include "syntax/lexer/abstract_lexer.h"

#include <cwctype>
#include <functional>
#include <string>

#include "syntax/lexer/token.h"

namespace orion::syntax {
bool AbstractLexer::IsCurrent(const char32_t ch, const size_t offset) const {
  if (AtEnd(offset)) {
    return false;
  }

  const size_t current = end_ + offset;
  return source_.at(current) == ch;
}

bool AbstractLexer::IsCurrent(const std::u32string& value,
                              const size_t offset) const {
  if (AtEnd(offset + value.size() - 1)) {
    return false;
  }

  const std::u32string substring = source_.substr(end_ + offset, value.size());

  return substring == value;
}

bool AbstractLexer::IsCurrent(const std::function<bool(char32_t)>& predicate,
                              const size_t offset) const {
  if (AtEnd(offset)) {
    return false;
  }

  const size_t current = end_ + offset;
  return predicate(source_.at(current));
}

bool AbstractLexer::IsCurrent2(const char32_t ch1, const char32_t ch2,
                               const size_t offset) const {
  return IsCurrent(ch1, offset) || IsCurrent(ch2, offset);
}

bool AbstractLexer::IsCurrent3(const char32_t ch1, const char32_t ch2,
                               const char32_t ch3, const size_t offset) const {
  return IsCurrent(ch1, offset) || IsCurrent(ch2, offset) ||
         IsCurrent(ch3, offset);
}

// Consume
void AbstractLexer::Consume(const size_t count) {
  size_t consumed = 0;
  while (!AtEnd() && consumed++ < count) {
    end_ += 1;
  }
}

void AbstractLexer::ConsumeIf(const bool condition) {
  if (!AtEnd() && condition) {
    Consume();
  }
}

void AbstractLexer::ConsumeWhile(
    const std::function<bool(char32_t)>& predicate) {
  while (!AtEnd() && predicate(source_.at(end_))) {
    end_++;
  }
}

void AbstractLexer::TryConsume(const char32_t ch) {
  if (!AtEnd() && source_.at(end_) == ch) {
    Consume();
  }
}

void AbstractLexer::TryConsume2(const char32_t ch1, const char32_t ch2) {
  if (!AtEnd() && (source_.at(end_) == ch1 || source_.at(end_) == ch2)) {
    Consume();
  }
}
}  // namespace orion::syntax
