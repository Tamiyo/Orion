#ifndef YUZU_PARSER_PARSE_ERROR_H
#define YUZU_PARSER_PARSE_ERROR_H

#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Util/ErrorHandling.h"

#include "llvm/Support/FormatVariadic.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace yuzu::parser {
struct [[nodiscard]] ExpectedKindError {
  const std::vector<lexer::TokenKind> expected;
  const std::optional<lexer::TokenKind> found;
  const lexer::Range range;
};

class [[nodiscard]] ParseError final : public std::variant<ExpectedKindError> {
public:
  using std::variant<ExpectedKindError>::variant;

  ParseError() = delete;

  std::string asString() const {
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

      return llvm::formatv(
                 "parser error at {0}, {1} - found {2} but expected one of {3}",
                 error->range.start, error->range.end, found, expected)
          .str();
    }

    util::yuzu_unreachable();
  }
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_PARSE_ERROR_H
