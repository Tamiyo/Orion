#ifndef SYNTAX_PARSER_ERROR_PARSE_ERROR_H_
#define SYNTAX_PARSER_ERROR_PARSE_ERROR_H_

#include <cstdint>
#include <optional>
#include <vector>

#include "syntax/lexer/span.h"

namespace yuzu::syntax {
template <typename TokenKind = uint16_t>
struct ParseError {
  const std::vector<TokenKind> expected;
  const std::optional<TokenKind> found;
  const Span span;

  bool operator==(const ParseError& other) const {
    return expected == other.expected && found == other.found &&
           span == other.span;
  }
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_ERROR_PARSE_ERROR_H_
