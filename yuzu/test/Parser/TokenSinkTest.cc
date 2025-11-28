#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Ast/SyntaxKind.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Parser/Event.h"

#include "gtest/gtest.h"

#include <optional>
#include <string>
#include <vector>

namespace {
using yuzu::ast::SyntaxKind;
using yuzu::lexer::Lexer;
using yuzu::lexer::Token;
using yuzu::parser::ErrorEvent;
using yuzu::parser::Event;
using yuzu::parser::FinishEvent;
using yuzu::parser::StartEvent;
using yuzu::parser::TokenEvent;
using yuzu::parser::TokenSink;

inline std::vector<Token> lex(const std::u32string &source) {
  auto lexer = Lexer(source);
  return lexer.getTokens();
}

TEST(TokenSinkTest, SingleNodeWithNoChildren) {
  const auto tokens = std::vector<Token>{};
  const auto events = std::vector<Event>{
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      FinishEvent{},
  };

  auto sink = TokenSink(tokens, events);
  const auto result = sink.finish();

  EXPECT_TRUE(result.errors.empty());
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(SyntaxKind::BinaryExpr),
            result.green.getKind());
  EXPECT_EQ(0, result.green.getNumChildren());
}

TEST(TokenSinkTest, SingleTokenEvent) {
  const auto tokens = lex(U"a");
  const auto events = std::vector<Event>{
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      TokenEvent{.kind = SyntaxKind::Ident},
      FinishEvent{},
  };

  auto sink = TokenSink(tokens, events);
  const auto result = sink.finish();

  EXPECT_TRUE(result.errors.empty());
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(SyntaxKind::BinaryExpr),
            result.green.getKind());
  EXPECT_EQ(1, result.green.getNumChildren());
}

TEST(TokenSinkTest, ErrorEventAddsError) {
  const auto tokens = std::vector<Token>{};
  const auto events = std::vector<Event>{
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      ErrorEvent{.message = "unexpected token"},
      FinishEvent{},
  };

  auto sink = TokenSink(tokens, events);
  const auto result = sink.finish();

  EXPECT_EQ(1, result.errors.size());
  EXPECT_EQ("unexpected token", result.errors[0]);
}

TEST(TokenSinkTest, MultipleErrorEvents) {
  const auto tokens = std::vector<Token>{};
  const auto events = std::vector<Event>{
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      ErrorEvent{.message = "first error"},
      ErrorEvent{.message = "second error"},
      FinishEvent{},
  };

  auto sink = TokenSink(tokens, events);
  const auto result = sink.finish();

  EXPECT_EQ(2, result.errors.size());
  EXPECT_EQ("first error", result.errors[0]);
  EXPECT_EQ("second error", result.errors[1]);
}

TEST(TokenSinkTest, NestedNodes) {
  const auto tokens = lex(U"a");
  const auto events = std::vector<Event>{
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      StartEvent{.forwardParent = std::nullopt,
                 .kind = SyntaxKind::LiteralExpr},
      TokenEvent{.kind = SyntaxKind::Ident},
      FinishEvent{},
      FinishEvent{},
  };

  auto sink = TokenSink(tokens, events);
  const auto result = sink.finish();

  EXPECT_TRUE(result.errors.empty());
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(SyntaxKind::BinaryExpr),
            result.green.getKind());
  EXPECT_EQ(1, result.green.getNumChildren());
}

TEST(TokenSinkTest, ForwardParentCreatesWrappingNode) {
  const auto tokens = lex(U"a");
  // Event 0: Start LiteralExpr (forwardParent = 2, points to event 2)
  // Event 1: Token
  // Event 2: Start BinaryExpr (will be started before LiteralExpr due to
  // forward parent) Event 3: Finish BinaryExpr Event 4: Finish LiteralExpr
  const auto events = std::vector<Event>{
      StartEvent{.forwardParent = 2, .kind = SyntaxKind::LiteralExpr},
      TokenEvent{.kind = SyntaxKind::Ident},
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      FinishEvent{},
      FinishEvent{},
  };

  auto sink = TokenSink(tokens, events);
  const auto result = sink.finish();

  EXPECT_TRUE(result.errors.empty());
  // The BinaryExpr wraps LiteralExpr due to forward parent
  EXPECT_EQ(static_cast<yuzu::syntax::SyntaxKind>(SyntaxKind::BinaryExpr),
            result.green.getKind());
}

TEST(TokenSinkTest, MultipleTokens) {
  const auto tokens = lex(U"a b");
  const auto events = std::vector<Event>{
      StartEvent{.forwardParent = std::nullopt, .kind = SyntaxKind::BinaryExpr},
      TokenEvent{.kind = SyntaxKind::Ident},
      TokenEvent{.kind = SyntaxKind::Ident},
      FinishEvent{},
  };

  auto sink = TokenSink(tokens, events);
  const auto result = sink.finish();

  EXPECT_TRUE(result.errors.empty());
  // 2 ident tokens + 1 whitespace trivia
  EXPECT_EQ(3, result.green.getNumChildren());
}
} // namespace
