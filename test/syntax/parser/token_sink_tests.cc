#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "syntax/lexer/span.h"
#include "syntax/lexer/token.h"
#include "syntax/parser/error/parse_error.h"
#include "syntax/parser/event.h"
#include "syntax/parser/token_sink.h"
#include "syntax/rgtree/green.h"

namespace {
enum class TokenKind : uint16_t { kPlus, kMinus, kWhitespace };
enum class SyntaxKind : uint16_t { kPlus, kMinus, kError };

using yuzu::syntax::ErrorEvent;
using yuzu::syntax::Event;
using yuzu::syntax::FinishEvent;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::ParseError;
using yuzu::syntax::PlaceholderEvent;
using yuzu::syntax::Span;
using yuzu::syntax::StartEvent;
using yuzu::syntax::Token;
using yuzu::syntax::TokenEvent;
using yuzu::syntax::TokenSink;

const auto kSpan = Span(0, 0);
constexpr auto kTokenKindPlus = TokenKind::kPlus;
constexpr auto kTokenKindMinus = TokenKind::kMinus;

constexpr auto kSyntaxKindPlus = SyntaxKind::kPlus;
constexpr auto kSyntaxKindMinus = SyntaxKind::kMinus;
constexpr auto kSyntaxKindError = SyntaxKind::kError;

const auto kTokenPlus = Token(kTokenKindPlus, kSpan, U"+");
const auto kTokenMinus = Token(kTokenKindMinus, kSpan, U"-");

const auto kGreenTokenPlus = GreenToken(kSyntaxKindPlus, U"+");
const auto kGreenTokenMinus = GreenToken(kSyntaxKindMinus, U"-");

const auto kParseError =
    ParseError<TokenKind>{.expected = std::vector<TokenKind>(),
                          .found = std::nullopt,
                          .span = Span(0, 0)};

bool IsTrivia(const TokenKind kind) { return kind == TokenKind::kWhitespace; }

TEST(TokenSinkTest, BuildSingleTokenNode) {
  const auto tokens = std::vector{kTokenPlus};

  const auto events = std::vector<Event<TokenKind, SyntaxKind>>{
      StartEvent(kSyntaxKindError), TokenEvent{}, ErrorEvent{kParseError},
      FinishEvent{}};

  auto sink = TokenSink<TokenKind, SyntaxKind>(tokens, events, IsTrivia);
  const auto [node, errors] = sink.Finish();

  EXPECT_EQ(kSyntaxKindError, node.Kind());

  const auto expected_children =
      std::vector<GreenElement<SyntaxKind>>{GreenElement(kGreenTokenPlus)};
  const auto actual_children = node.Children();
  ASSERT_EQ(expected_children.size(), actual_children.size());

  const GreenElement<SyntaxKind>& expected_child = expected_children[0];
  const GreenElement<SyntaxKind>& actual_child = expected_children[0];
  ASSERT_EQ(expected_child.IsToken(), actual_child.IsToken());

  const GreenToken<SyntaxKind> expected_token =
      expected_child.TryGetToken().value();
  const GreenToken<SyntaxKind> actual_token =
      actual_child.TryGetToken().value();
  EXPECT_EQ(expected_token, actual_token);

  ASSERT_EQ(1, errors.size());
  EXPECT_EQ(kParseError, errors.front());
}

TEST(TokenSinkTest, BuildMultiTokenNode) {
  const auto tokens = std::vector{kTokenPlus, kTokenMinus};

  const auto events = std::vector<Event<TokenKind, SyntaxKind>>{
      StartEvent(kSyntaxKindError), TokenEvent{}, TokenEvent{}, FinishEvent{}};

  auto sink = TokenSink<TokenKind, SyntaxKind>(tokens, events, IsTrivia);
  const auto [node, errors] = sink.Finish();

  EXPECT_EQ(kSyntaxKindError, node.Kind());

  const auto expected_children = std::vector<GreenElement<SyntaxKind>>{
      GreenElement(kGreenTokenPlus),
      GreenElement(kGreenTokenMinus),
  };
  const auto actual_children = node.Children();
  ASSERT_EQ(expected_children.size(), actual_children.size());

  for (size_t i = 0; i < expected_children.size(); i++) {
    const GreenElement<SyntaxKind>& expected_child = expected_children[0];
    const GreenElement<SyntaxKind>& actual_child = expected_children[0];
    ASSERT_EQ(expected_child.IsToken(), actual_child.IsToken());

    const GreenToken<SyntaxKind> expected_token =
        expected_child.TryGetToken().value();
    const GreenToken<SyntaxKind> actual_token =
        actual_child.TryGetToken().value();
    EXPECT_EQ(expected_token, actual_token);
  }

  EXPECT_EQ(0, errors.size());
}
}  // namespace
