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
using yuzu::parser::ExpectedExpressionError;
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

TEST(ExpectedKindErrorTest, FormatsFoundTokenByDisplayName) {
  // Single expected: "expected <X>, found <Y>". Both names go through
  // `asDisplayString`, so `Ident` reads as the schema's `Display`
  // override ("identifier") and `Plus` falls back to the single-value
  // `Token` default (`` `+` ``).
  const ExpectedKindError error(std::vector<TokenKind>{TokenKind::Plus},
                                TokenKind::Ident, Range{.start = 3, .end = 8});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ("expected `+`, found identifier", diag.message);
}

TEST(ExpectedKindErrorTest, ReportsEndOfInputWhenFoundIsMissing) {
  // At end-of-input, `found` is nullopt; the formatter says "end of
  // input" instead of leaking the optional's missing state into prose.
  const ExpectedKindError error(std::vector<TokenKind>{TokenKind::Plus},
                                std::nullopt, Range{.start = 3, .end = 8});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ("expected `+`, found end of input", diag.message);
}

TEST(ExpectedKindErrorTest, JoinsMultipleExpectedKindsWithComma) {
  // More than one expected kind switches to "expected one of <a>, <b>".
  const ExpectedKindError error(
      std::vector<TokenKind>{TokenKind::Plus, TokenKind::Minus},
      TokenKind::Ident, Range{.start = 0, .end = 1});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ("expected one of `+`, `-`, found identifier", diag.message);
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

TEST(ExpectedExpressionErrorTest, QuotesFoundTokenText) {
  const ExpectedExpressionError error(U"^", Range{.start = 2, .end = 3});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ(Severity::Error, diag.severity);
  EXPECT_EQ("expected expression, found `^`", diag.message);
}

TEST(ExpectedExpressionErrorTest, ReportsEndOfInputWhenTextIsEmpty) {
  // Empty `foundText` is the EOF signal — at end-of-input the parser
  // hasn't got a real token to quote.
  const ExpectedExpressionError error(U"", Range{.start = 2, .end = 3});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ("expected expression, found end of input", diag.message);
}

TEST(ExpectedExpressionErrorTest, EncodesNonAsciiFoundTextAsUtf8) {
  // The `foundText` is UTF-32; the message embeds it as UTF-8 in
  // backticks. Sanity-check with a non-ASCII code point so we know the
  // encoding goes through `util::toUtf8` correctly.
  const ExpectedExpressionError error(U"é", Range{.start = 0, .end = 1});

  const auto diag = error.toDiagnostic(kTestSource);
  EXPECT_EQ("expected expression, found `\xC3\xA9`", diag.message);
}

TEST(ExpectedExpressionErrorTest, EmitsPrimaryLabelOverGivenSpan) {
  const ExpectedExpressionError error(U"^", Range{.start = 2, .end = 3});

  const auto diag = error.toDiagnostic(kTestSource);
  ASSERT_EQ(1u, diag.labels.size());
  EXPECT_EQ(LabelStyle::Primary, diag.labels[0].style);
  EXPECT_EQ(kTestSource, diag.labels[0].span.source);
  EXPECT_EQ(2u, diag.labels[0].span.start);
  EXPECT_EQ(3u, diag.labels[0].span.end);
}

} // namespace
