#ifndef YUZU_PARSER_PARSE_ERROR_H
#define YUZU_PARSER_PARSE_ERROR_H

#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/TokenKind.h"

#include "llvm/Support/FormatVariadic.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace yuzu::parser {
class [[nodiscard]] ParseError {
public:
  // Virtual destructor is crucial for abstract base classes
  virtual ~ParseError() = default;

  // Pure virtual method makes the class abstract
  virtual std::string asString() const = 0;
};

class [[nodiscard]] ExpectedKindError final : public ParseError {
public:
  explicit ExpectedKindError(std::vector<lexer::TokenKind> expected,
                             std::optional<lexer::TokenKind> found,
                             lexer::Range range)
      : expected(std::move(expected)), found(std::move(found)),
        range(std::move(range)) {}

  ExpectedKindError() = delete;

  std::string asString() const override {
    const std::string expectedKindAsString =
        found ? lexer::asString(*found) : "None";

    std::string expectedKinds = "[";
    for (size_t i = 0, size = expected.size(); i < size; i++) {
      expectedKinds.append(lexer::asString(expected[i]));
      if (i < size - 1) {
        expectedKinds.append(", ");
      }
    }
    expectedKinds.append("]");

    return llvm::formatv(
               "parser error at {0}, {1} - found {2} but expected one of {3}",
               range.start, range.end, expectedKindAsString, expectedKinds)
        .str();
  }

private:
  std::vector<lexer::TokenKind> expected;
  std::optional<lexer::TokenKind> found;
  lexer::Range range;
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_PARSE_ERROR_H
