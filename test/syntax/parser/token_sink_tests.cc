#include <gtest/gtest.h>

#include <vector>

#include "syntax/lexer/span.h"
#include "syntax/lexer/token.h"
#include "syntax/lexer/token_kind.h"
#include "syntax/parser/event.h"
#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/rgtree/green/green_node.h"
#include "syntax/parser/rgtree/green/green_token.h"
#include "syntax/parser/token_sink.h"
#include "syntax/syntax_kind.h"

namespace {
using orion::syntax::Event;
using orion::syntax::GreenElement;
using orion::syntax::GreenNode;
using orion::syntax::GreenToken;
using orion::syntax::Span;
using orion::syntax::SyntaxKind;
using orion::syntax::Token;
using orion::syntax::TokenKind;
using orion::syntax::TokenSink;

const auto kSpan = Span(0, 0);
constexpr auto kTokenKindPlus = TokenKind::kPlus;
constexpr auto kTokenKindMinus = TokenKind::kMinus;

constexpr auto kSyntaxKindPlus = SyntaxKind::kPlus;
constexpr auto kSyntaxKindMinus = SyntaxKind::kMinus;
constexpr auto kSyntaxKindError = SyntaxKind::kError;

const auto kTokenPlus =
    Token(static_cast<uint16_t>(kTokenKindPlus), kSpan, U"+");

const auto kTokenMinus =
    Token(static_cast<uint16_t>(kTokenKindMinus), kSpan, U"-");

const auto kGreenTokenPlus = GreenToken(kSyntaxKindPlus, U"+");
const auto kGreenTokenMinus = GreenToken(kSyntaxKindMinus, U"-");

TEST(TokenSinkTest, BuildSingleTokenNode) {
  const auto tokens = std::vector<Token>{kTokenPlus};

  const auto events =
      std::vector<Event>{Event::CreateStart(kSyntaxKindError),
                         Event::CreateToken(), Event::CreateFinish()};

  auto sink = TokenSink(tokens, events);
  const auto [node] = sink.Finish();

  EXPECT_EQ(kSyntaxKindError, node.Kind());

  const auto expected_children =
      std::vector<GreenElement>{GreenElement(kGreenTokenPlus)};
  const auto actual_children = node.Children();
  ASSERT_EQ(expected_children.size(), actual_children.size());

  const GreenElement& expected_child = expected_children[0];
  const GreenElement& actual_child = expected_children[0];
  ASSERT_EQ(expected_child.IsToken(), actual_child.IsToken());

  const GreenToken expected_token = expected_child.TryGetToken().value();
  const GreenToken actual_token = actual_child.TryGetToken().value();
  EXPECT_EQ(expected_token, actual_token);
}

TEST(TokenSinkTest, BuildMultiTokenNode) {
  const auto tokens = std::vector<Token>{kTokenPlus, kTokenMinus};

  const auto events = std::vector<Event>{
      Event::CreateStart(kSyntaxKindError), Event::CreateToken(),
      Event::CreateToken(), Event::CreateFinish()};

  auto sink = TokenSink(tokens, events);
  const auto [node] = sink.Finish();

  EXPECT_EQ(kSyntaxKindError, node.Kind());

  const auto expected_children = std::vector<GreenElement>{
      GreenElement(kGreenTokenPlus),
      GreenElement(kGreenTokenMinus),
  };
  const auto actual_children = node.Children();
  ASSERT_EQ(expected_children.size(), actual_children.size());

  for (size_t i = 0; i < expected_children.size(); i++) {
    const GreenElement& expected_child = expected_children[0];
    const GreenElement& actual_child = expected_children[0];
    ASSERT_EQ(expected_child.IsToken(), actual_child.IsToken());

    const GreenToken expected_token = expected_child.TryGetToken().value();
    const GreenToken actual_token = actual_child.TryGetToken().value();
    EXPECT_EQ(expected_token, actual_token);
  }
}
}  // namespace
