#include "yuzu/Parser/ParseError.h"

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/TokenKind.h"

#include <gtest/gtest.h>

#include <optional>
#include <vector>

namespace {
using yuzu::diagnostics::LabelStyle;
using yuzu::diagnostics::Severity;
using yuzu::diagnostics::SourceId;
using yuzu::lexer::Range;
using yuzu::lexer::TokenKind;
using yuzu::parser::ExpectedKindError;

// Tag the tests' synthetic spans with a non-zero SourceId so they don't
// alias the default-constructed sentinel.
constexpr SourceId kTestSource{1};

TEST(ExpectedKindErrorTest, ProducesErrorSeverityDiagnostic) {
  const ExpectedKindError error(std::vector<TokenKind>{TokenKind::Plus},
                                TokenKind::Ident, Range{.start = 3, .end = 8});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ(Severity::Error, diag.severity);
}

TEST(ExpectedKindErrorTest, FormatsFoundTokenByName) {
  const ExpectedKindError error(std::vector<TokenKind>{TokenKind::Plus},
                                TokenKind::Ident, Range{.start = 3, .end = 8});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ("found Ident but expected one of [Plus]", diag.message);
}

TEST(ExpectedKindErrorTest, FormatsMissingFoundTokenAsNone) {
  // When the parser hits end-of-input, `found` is nullopt; the formatter
  // surfaces that as the literal "None".
  const ExpectedKindError error(std::vector<TokenKind>{TokenKind::Plus},
                                std::nullopt, Range{.start = 3, .end = 8});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ("found None but expected one of [Plus]", diag.message);
}

TEST(ExpectedKindErrorTest, JoinsMultipleExpectedKindsWithComma) {
  const ExpectedKindError error(
      std::vector<TokenKind>{TokenKind::Plus, TokenKind::Minus},
      TokenKind::Ident, Range{.start = 0, .end = 1});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ("found Ident but expected one of [Plus, Minus]", diag.message);
}

TEST(ExpectedKindErrorTest, EmitsPrimaryLabelOverGivenSpan) {
  const ExpectedKindError error(std::vector<TokenKind>{TokenKind::Plus},
                                TokenKind::Ident, Range{.start = 3, .end = 8});

  const auto diag = error.toDiagnostic(kTestSource);
  ASSERT_EQ(1u, diag.labels.size());
  EXPECT_EQ(LabelStyle::Primary, diag.labels[0].style);
  EXPECT_EQ(kTestSource, diag.labels[0].span.source);
  EXPECT_EQ(3u, diag.labels[0].span.start);
  EXPECT_EQ(8u, diag.labels[0].span.end);
}

} // namespace
