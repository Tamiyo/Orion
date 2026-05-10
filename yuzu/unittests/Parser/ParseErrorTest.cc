#include "yuzu/Parser/ParseError.h"

#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/TokenKind.h"

#include <gtest/gtest.h>

#include <optional>
#include <vector>

namespace {
using yuzu::lexer::Range;
using yuzu::lexer::TokenKind;
using yuzu::parser::ExpectedKindError;

TEST(ParseErrorTest, FormatsFoundTokenByName) {
  const ExpectedKindError error(std::vector<TokenKind>{TokenKind::Plus},
                                TokenKind::Ident, Range{.start = 3, .end = 8});

  EXPECT_EQ("parser error at 3, 8 - found Ident but expected one of [Plus]",
            error.asString());
}

TEST(ParseErrorTest, FormatsMissingFoundTokenAsNone) {
  // When the parser hits end-of-input, `found` is nullopt; the formatter
  // must surface that as the literal "None".
  const ExpectedKindError error(std::vector<TokenKind>{TokenKind::Plus},
                                std::nullopt, Range{.start = 3, .end = 8});

  EXPECT_EQ("parser error at 3, 8 - found None but expected one of [Plus]",
            error.asString());
}

TEST(ParseErrorTest, JoinsMultipleExpectedKindsWithComma) {
  const ExpectedKindError error(
      std::vector<TokenKind>{TokenKind::Plus, TokenKind::Minus},
      TokenKind::Ident, Range{.start = 0, .end = 1});

  EXPECT_EQ(
      "parser error at 0, 1 - found Ident but expected one of [Plus, Minus]",
      error.asString());
}

} // namespace
