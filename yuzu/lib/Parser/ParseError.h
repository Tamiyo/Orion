#ifndef YUZU_PARSER_PARSE_ERROR_H
#define YUZU_PARSER_PARSE_ERROR_H

#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Util/ErrorHandling.h"

#include <fmt/core.h>

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace yuzu::parser {
struct ExpectedKindError {
  const std::vector<lexer::TokenKind> expected;
  const std::optional<lexer::TokenKind> found;
  const lexer::Range range;
};

class ParseError final : public std::variant<ExpectedKindError> {
public:
  using std::variant<ExpectedKindError>::variant;

  ParseError() = delete;

  std::string asString() const noexcept {
    if (const ExpectedKindError *error = std::get_if<ExpectedKindError>(this)) {
      const std::string found = lexer::asString(error->found);

      std::string expected = "[";
      for (size_t i = 0, size = error->expected.size(); i < size; i++) {
        expected.append(lexer::asString(error->expected[i]));
        if (i < size - 1) {
          expected.append(", ");
        }
      }
      expected.append("]");

      return fmt::format(
          "parser error at {}, {} - found {} but expected one of {}",
          error->range.start, error->range.end, found, expected);
    }

    util::yuzu_unreachable();
  }
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_PARSE_ERROR_H