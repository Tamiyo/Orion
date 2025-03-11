#include "syntax/lexer/abstract_lexer.h"

#include <cwctype>
#include <functional>
#include <string>

#include "syntax/lexer/token.h"

namespace orion::syntax {
bool AbstractLexer::AtEnd(const size_t offset) const {
  return end_ + offset >= source_length_;
}

char AbstractLexer::GetCurrent() const { return source_[end_]; }

bool AbstractLexer::IsCurrent(const char ch, const size_t offset) const {
  if (AtEnd(offset)) {
    return false;
  }

  const size_t current = end_ + offset;
  if (is_case_sensitive_) {
    return source_[current] == ch;
  }

  return std::toupper(source_[current]) == std::toupper(ch);
}

bool AbstractLexer::IsCurrent(const std::function<int(int)>& predicate,
                              const size_t offset) const {
  if (AtEnd(offset)) {
    return false;
  }

  const size_t current = end_ + offset;
  if (is_case_sensitive_) {
    return predicate(source_[current]);
  }

  return predicate(std::toupper(source_[current]));
}

bool AbstractLexer::IsCurrent2(const char ch1, const char ch2,
                               const size_t offset) const {
  return IsCurrent(ch1, offset) || IsCurrent(ch2, offset);
}

bool AbstractLexer::IsCurrent3(const char ch1, const char ch2, const char ch3,
                               const size_t offset) const {
  return IsCurrent(ch1, offset) || IsCurrent(ch2, offset) ||
         IsCurrent(ch3, offset);
}

bool AbstractLexer::IsCurrentSubstr(const std::string& substr) const {
  const size_t offset = substr.length();
  if (AtEnd(end_ + offset)) {
    return false;
  }

  const std::string source_substr = source_.substr(end_, offset);
  if (source_substr.length() != substr.length()) {
    return false;
  }

  for (int idx = 0; idx < source_substr.length(); idx++) {
    const char s1 = is_case_sensitive_ ? source_substr[idx]
                                       : std::toupper(source_substr[idx]);
    const char s2 =
        is_case_sensitive_ ? substr[idx] : std::toupper(source_substr[idx]);

    if (s1 != s2) {
      return false;
    }
  }

  return true;
}

// Consume
void AbstractLexer::Consume(const size_t count) {
  size_t consumed = 0;
  while (!AtEnd() && consumed++ < count) {
    end_ += 1;
  }
}

void AbstractLexer::ConsumeIf(const bool condition) {
  if (AtEnd()) {
    return;
  }

  if (condition) {
    Consume();
  }
}

void AbstractLexer::ConsumeWhile(const std::function<bool(char)>& predicate) {
  if (AtEnd()) {
    return;
  }

  while (predicate(source_[end_])) {
    end_++;
  }
}

void AbstractLexer::TryConsume(const char ch) {
  if (AtEnd()) {
    return;
  }

  if (source_[end_] == ch) {
    Consume();
  }
}

void AbstractLexer::TryConsume2(const char ch1, const char ch2) {
  if (source_[end_] == ch1 || source_[end_] == ch2) {
    Consume();
  }
}
}  // namespace orion::syntax
