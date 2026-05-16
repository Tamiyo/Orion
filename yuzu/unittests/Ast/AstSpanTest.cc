#include "yuzu/Ast/AstSpan.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"

#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <utility>

namespace {

using yuzu::ast::BinaryExpr;
using yuzu::ast::ExprStmt;
using yuzu::ast::IntLit;
using yuzu::ast::Root;
using yuzu::ast::SyntaxNode;
using yuzu::ast::tightRange;

class AstSpanTest : public ::testing::Test {
protected:
  yuzu::diagnostics::SourceMap sources;
  yuzu::diagnostics::DiagnosticsEngine diagnostics;

  // Lex + parse `source` as a full root and return the typed AST root.
  Root parseRoot(std::u32string_view source) {
    auto lexer = yuzu::lexer::Lexer(source);
    auto tokens = lexer.getTokens();

    const auto sourceId = sources.add("<test>", std::u32string(source));

    auto parser = yuzu::parser::Parser(yuzu::parser::TokenSource(tokens));
    yuzu::parser::parseRoot(parser);

    auto events = std::move(parser).finish();
    auto sink = yuzu::parser::TokenSink(std::move(tokens), std::move(events),
                                        diagnostics, sourceId);
    auto sinkResult = sink.finish();
    return Root{SyntaxNode::createRoot(sinkResult.green)};
  }
};

TEST_F(AstSpanTest, TrimsTrailingWhitespaceOwnedByNode) {
  // FloatLit absorbs the trailing space between `2.1` and `+`, so its
  // raw range is `0..4`. `tightRange` should give back just the digits.
  const Root root = parseRoot(U"2.1 + 2");
  const auto stmt = *root.getStmts().begin();
  const auto exprStmt = ExprStmt::cast(stmt);
  ASSERT_TRUE(exprStmt.has_value());
  const auto expr = exprStmt->getExpr();
  ASSERT_TRUE(expr.has_value());
  const auto bin = BinaryExpr::cast(*expr);
  ASSERT_TRUE(bin.has_value());
  const auto lhs = bin->getLhs();
  ASSERT_TRUE(lhs.has_value());

  const auto raw = lhs->getRange();
  EXPECT_EQ(raw.start, 0u);
  EXPECT_EQ(raw.end, 4u);

  const auto tight = tightRange(*lhs);
  EXPECT_EQ(tight.start, 0u);
  EXPECT_EQ(tight.end, 3u);
}

TEST_F(AstSpanTest, NoTriviaLeavesRangeAlone) {
  // `2` at the tail of an expression has no trailing trivia — tight and
  // raw should agree.
  const Root root = parseRoot(U"2.1 + 2");
  const auto stmt = *root.getStmts().begin();
  const auto exprStmt = ExprStmt::cast(stmt);
  ASSERT_TRUE(exprStmt.has_value());
  const auto expr = exprStmt->getExpr();
  ASSERT_TRUE(expr.has_value());
  const auto bin = BinaryExpr::cast(*expr);
  ASSERT_TRUE(bin.has_value());
  const auto rhs = bin->getRhs();
  ASSERT_TRUE(rhs.has_value());
  const auto lit = IntLit::cast(*rhs);
  ASSERT_TRUE(lit.has_value());

  EXPECT_EQ(lit->getRange().end, tightRange(*lit).end);
}

TEST_F(AstSpanTest, TrimsTrailingNewline) {
  // Trailing newline owned by the node should be stripped.
  const Root root = parseRoot(U"42\n");
  const auto stmt = *root.getStmts().begin();
  const auto exprStmt = ExprStmt::cast(stmt);
  ASSERT_TRUE(exprStmt.has_value());
  const auto expr = exprStmt->getExpr();
  ASSERT_TRUE(expr.has_value());
  const auto lit = IntLit::cast(*expr);
  ASSERT_TRUE(lit.has_value());

  const auto tight = tightRange(*lit);
  EXPECT_EQ(tight.start, 0u);
  EXPECT_EQ(tight.end, 2u); // just "42", no `\n`
}

TEST_F(AstSpanTest, TrimsTriviaOwnedByDeepDescendant) {
  // For a BinaryExpr, the rightmost leaf is the rhs literal. Trailing
  // trivia owned by that descendant should still be stripped by the
  // recursive walk.
  const Root root = parseRoot(U"1 + 2\n");
  const auto stmt = *root.getStmts().begin();
  const auto exprStmt = ExprStmt::cast(stmt);
  ASSERT_TRUE(exprStmt.has_value());
  const auto expr = exprStmt->getExpr();
  ASSERT_TRUE(expr.has_value());
  const auto bin = BinaryExpr::cast(*expr);
  ASSERT_TRUE(bin.has_value());

  const auto tight = tightRange(*bin);
  EXPECT_EQ(tight.start, 0u);
  EXPECT_EQ(tight.end, 5u); // "1 + 2", no `\n`
}

} // namespace
