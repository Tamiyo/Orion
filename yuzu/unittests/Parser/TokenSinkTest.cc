#include "yuzu/Parser/TokenSink.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/ParseError.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <utility>
#include <vector>

namespace {
using yuzu::ast::SyntaxKind;
using yuzu::diagnostics::DiagnosticsEngine;
using yuzu::diagnostics::Severity;
using yuzu::diagnostics::SourceId;
using yuzu::diagnostics::SourceMap;
using yuzu::lexer::Lexer;
using yuzu::lexer::Range;
using yuzu::lexer::Token;
using yuzu::lexer::TokenKind;
using yuzu::parser::ErrorEvent;
using yuzu::parser::Event;
using yuzu::parser::ExpectedKindError;
using yuzu::parser::FinishEvent;
using yuzu::parser::ParseError;
using yuzu::parser::StartEvent;
using yuzu::parser::TokenEvent;
using yuzu::parser::TokenSink;

inline std::vector<Token> lex(const std::u32string &source) {
  auto lexer = Lexer(source);
  return lexer.getTokens();
}

// Build a `std::vector<Event>` from a parameter pack. `Event` is move-only
// (it can hold a `unique_ptr<ParseError>` via ErrorEvent), so the natural
// brace-init form `std::vector<Event>{a, b, c}` would copy and fail to
// compile. This helper emplaces each argument in turn.
template <typename... Es> std::vector<Event> makeEvents(Es &&...events) {
  std::vector<Event> out;
  out.reserve(sizeof...(Es));
  (out.emplace_back(std::forward<Es>(events)), ...);
  return out;
}

// `unique_ptr<ParseError>` is move-only, so each test that consumes one
// needs a freshly-constructed instance.
std::unique_ptr<ParseError> makeError1() {
  return std::make_unique<ExpectedKindError>(
      std::vector<TokenKind>{TokenKind::Plus}, TokenKind::Ident,
      Range{.start = 3, .end = 8});
}

std::unique_ptr<ParseError> makeError2() {
  return std::make_unique<ExpectedKindError>(
      std::vector<TokenKind>{TokenKind::Minus}, TokenKind::Ident,
      Range{.start = 2, .end = 7});
}

class TokenSinkTest : public ::testing::Test {
protected:
  SourceMap sources;
  DiagnosticsEngine engine;
  SourceId sourceId;

  void SetUp() override {
    // Register a synthetic source so the sink has somewhere to attribute
    // diagnostics. The text doesn't matter — the tests don't assert on
    // rendered output, only on the structured diagnostic shape.
    sourceId = sources.add("<test>", U"");
  }
};

TEST_F(TokenSinkTest, SingleNodeWithNoChildren) {
  const auto tokens = std::vector<Token>{};
  auto events = makeEvents(
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      FinishEvent{});

  auto sink = TokenSink(tokens, std::move(events), engine, sourceId);
  const auto result = sink.finish();

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(SyntaxKind::BinaryExpr),
            result.green.getKind());
  EXPECT_EQ(0, result.green.getNumChildren());
}

TEST_F(TokenSinkTest, SingleTokenEvent) {
  const auto tokens = lex(U"a");
  auto events = makeEvents(
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      TokenEvent{.kind = SyntaxKind::Ident}, FinishEvent{});

  auto sink = TokenSink(tokens, std::move(events), engine, sourceId);
  const auto result = sink.finish();

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(SyntaxKind::BinaryExpr),
            result.green.getKind());
  EXPECT_EQ(1, result.green.getNumChildren());
}

TEST_F(TokenSinkTest, ErrorEventEmitsDiagnostic) {
  const auto tokens = std::vector<Token>{};
  auto events = makeEvents(
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      ErrorEvent{.error = makeError1()}, FinishEvent{});

  auto sink = TokenSink(tokens, std::move(events), engine, sourceId);
  const auto _ = sink.finish();

  ASSERT_EQ(1u, engine.getDiagnostics().size());
  const auto &d = engine.getDiagnostics()[0];
  EXPECT_EQ(Severity::Error, d.severity);
  EXPECT_EQ("found Ident but expected one of [Plus]", d.message);
  ASSERT_EQ(1u, d.labels.size());
  EXPECT_EQ(sourceId, d.labels[0].span.source);
  EXPECT_EQ(3u, d.labels[0].span.start);
  EXPECT_EQ(8u, d.labels[0].span.end);
}

TEST_F(TokenSinkTest, MultipleErrorEvents) {
  const auto tokens = std::vector<Token>{};
  auto events = makeEvents(
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      ErrorEvent{.error = makeError1()}, ErrorEvent{.error = makeError2()},
      FinishEvent{});

  auto sink = TokenSink(tokens, std::move(events), engine, sourceId);
  const auto _ = sink.finish();

  ASSERT_EQ(2u, engine.getDiagnostics().size());
  EXPECT_EQ("found Ident but expected one of [Plus]",
            engine.getDiagnostics()[0].message);
  EXPECT_EQ("found Ident but expected one of [Minus]",
            engine.getDiagnostics()[1].message);
}

TEST_F(TokenSinkTest, NestedNodes) {
  const auto tokens = lex(U"a");
  auto events = makeEvents(
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      StartEvent{.forwardParent = std::nullopt,
                 .kind = SyntaxKind::IntLit},
      TokenEvent{.kind = SyntaxKind::Ident}, FinishEvent{}, FinishEvent{});

  auto sink = TokenSink(tokens, std::move(events), engine, sourceId);
  const auto result = sink.finish();

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(SyntaxKind::BinaryExpr),
            result.green.getKind());
  EXPECT_EQ(1, result.green.getNumChildren());
}

TEST_F(TokenSinkTest, ForwardParentCreatesWrappingNode) {
  const auto tokens = lex(U"a");
  // Event 0: Start LiteralExpr (forwardParent = 2, points to event 2)
  // Event 1: Token
  // Event 2: Start BinaryExpr (will be started before LiteralExpr due to
  // forward parent) Event 3: Finish BinaryExpr Event 4: Finish LiteralExpr
  auto events = makeEvents(
      StartEvent{.forwardParent = 2, .kind = SyntaxKind::IntLit},
      TokenEvent{.kind = SyntaxKind::Ident},
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      FinishEvent{}, FinishEvent{});

  auto sink = TokenSink(tokens, std::move(events), engine, sourceId);
  const auto result = sink.finish();

  EXPECT_TRUE(engine.getDiagnostics().empty());
  // The BinaryExpr wraps LiteralExpr due to forward parent.
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(SyntaxKind::BinaryExpr),
            result.green.getKind());
}

TEST_F(TokenSinkTest, MultipleTokens) {
  const auto tokens = lex(U"a b");
  auto events = makeEvents(
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      TokenEvent{.kind = SyntaxKind::Ident},
      TokenEvent{.kind = SyntaxKind::Ident}, FinishEvent{});

  auto sink = TokenSink(tokens, std::move(events), engine, sourceId);
  const auto result = sink.finish();

  EXPECT_TRUE(engine.getDiagnostics().empty());
  // 2 ident tokens + 1 whitespace trivia
  EXPECT_EQ(3, result.green.getNumChildren());
}
} // namespace
