#include "yuzu/Hir/HirContext.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Hir/Hir.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"

#include <llvm/ADT/ArrayRef.h>

#include <gtest/gtest.h>

#include <array>
#include <string>
#include <string_view>
#include <utility>

namespace {

using namespace yuzu::hir;
using yuzu::ast::BinaryExpr;
using yuzu::ast::ExprStmt;
using yuzu::ast::Root;
using yuzu::ast::SyntaxNode;
using yuzu::diagnostics::DiagnosticsEngine;
using yuzu::diagnostics::SourceId;
using yuzu::diagnostics::SourceMap;

class HirContextTest : public ::testing::Test {
protected:
  SourceMap sources;
  DiagnosticsEngine diagnostics;

  struct Parsed {
    Root root;
    SourceId sourceId;
  };

  /// Lex + parse `source` and register it with the source map. Returns
  /// the typed root and the freshly minted `SourceId` so the test can
  /// construct an `HirContext` bound to the same source.
  Parsed parse(std::u32string_view source) {
    auto lexer = yuzu::lexer::Lexer(source);
    auto tokens = lexer.getTokens();

    const auto sourceId = sources.add("<test>", std::u32string(source));

    auto parser = yuzu::parser::Parser(yuzu::parser::TokenSource(tokens));
    yuzu::parser::parseRoot(parser);

    auto events = std::move(parser).finish();
    auto sink = yuzu::parser::TokenSink(std::move(tokens), std::move(events),
                                        diagnostics, sourceId);
    auto sinkResult = sink.finish();
    return Parsed{Root{SyntaxNode::createRoot(sinkResult.green)}, sourceId};
  }

  /// Pull the lhs / rhs operands of the single top-level `BinaryExpr`
  /// out of a parsed root. Returns the two operand `ast::Expr` views;
  /// the test fails the surrounding `ASSERT_TRUE`s if the source didn't
  /// parse as expected.
  struct BinaryOperands {
    yuzu::ast::Expr lhs;
    yuzu::ast::Expr rhs;
  };
  static BinaryOperands operandsOf(const Root &root) {
    const auto stmt = *root.getStmts().begin();
    const auto exprStmt = ExprStmt::cast(stmt);
    EXPECT_TRUE(exprStmt.has_value());
    const auto expr = exprStmt->getExpr();
    EXPECT_TRUE(expr.has_value());
    const auto bin = BinaryExpr::cast(*expr);
    EXPECT_TRUE(bin.has_value());
    const auto lhs = bin->getLhs();
    EXPECT_TRUE(lhs.has_value());
    const auto rhs = bin->getRhs();
    EXPECT_TRUE(rhs.has_value());
    return BinaryOperands{*lhs, *rhs};
  }
};

//===----------------------------------------------------------------------===//
// Single-node spans
//===----------------------------------------------------------------------===//

TEST_F(HirContextTest, SpanForUnboundIdReturnsZeroSpanInCtxSource) {
  // An HIR node whose id was never bound to an AST origin still gets a
  // well-formed span — start == end == 0, source == ctx's source.
  const auto [root, sourceId] = parse(U"1 + 2");
  HirContext ctx{diagnostics, sourceId};
  const auto *lit =
      ctx.getBuilder().makeIntLit(0);

  const auto span = ctx.spanFor(lit);
  EXPECT_EQ(span.source, sourceId);
  EXPECT_EQ(span.start, 0u);
  EXPECT_EQ(span.end, 0u);
}

TEST_F(HirContextTest, SpanForBoundNodeReturnsTightRangeOfAstOrigin) {
  // `1` sits at offsets 0..1 in `1 + 2`. After binding the HIR node's
  // id to that AST operand, `spanFor(node)` should report exactly that.
  const auto [root, sourceId] = parse(U"1 + 2");
  HirContext ctx{diagnostics, sourceId};
  const auto operands = operandsOf(root);
  const auto *lit =
      ctx.getBuilder().makeIntLit(0);
  ctx.getSourceTable().bind(lit->getId(), operands.lhs);

  const auto span = ctx.spanFor(lit);
  EXPECT_EQ(span.source, sourceId);
  EXPECT_EQ(span.start, 0u);
  EXPECT_EQ(span.end, 1u);
}

TEST_F(HirContextTest, SpanForHirIdAgreesWithSpanForNode) {
  // The id and node overloads must report the same span — the node
  // version is sugar over `node->getId()`.
  const auto [root, sourceId] = parse(U"1 + 2");
  HirContext ctx{diagnostics, sourceId};
  const auto operands = operandsOf(root);
  const auto *lit =
      ctx.getBuilder().makeIntLit(0);
  ctx.getSourceTable().bind(lit->getId(), operands.rhs);

  const auto a = ctx.spanFor(lit);
  const auto b = ctx.spanFor(lit->getId());
  EXPECT_EQ(a.source, b.source);
  EXPECT_EQ(a.start, b.start);
  EXPECT_EQ(a.end, b.end);
}

//===----------------------------------------------------------------------===//
// ArrayRef spans
//===----------------------------------------------------------------------===//

TEST_F(HirContextTest, SpanForEmptyArrayReturnsZeroSpan) {
  const auto [root, sourceId] = parse(U"1 + 2");
  HirContext ctx{diagnostics, sourceId};

  const auto span = ctx.spanFor(llvm::ArrayRef<const HirNode *>{});
  EXPECT_EQ(span.source, sourceId);
  EXPECT_EQ(span.start, 0u);
  EXPECT_EQ(span.end, 0u);
}

TEST_F(HirContextTest, SpanForSingleElementArrayMatchesSingleNodeSpan) {
  const auto [root, sourceId] = parse(U"1 + 2");
  HirContext ctx{diagnostics, sourceId};
  const auto operands = operandsOf(root);
  const auto *lit =
      ctx.getBuilder().makeIntLit(0);
  ctx.getSourceTable().bind(lit->getId(), operands.lhs);

  const std::array<const HirNode *, 1> one = {lit};
  const auto array = ctx.spanFor(llvm::ArrayRef<const HirNode *>{one});
  const auto single = ctx.spanFor(lit);
  EXPECT_EQ(array.source, single.source);
  EXPECT_EQ(array.start, single.start);
  EXPECT_EQ(array.end, single.end);
}

TEST_F(HirContextTest, SpanForTwoElementArrayCoversFirstStartToLastEnd) {
  // `1` is at 0..1 and `2` is at 4..5. The stitched span should run
  // 0..5 — covering the operator that sits between them too.
  const auto [root, sourceId] = parse(U"1 + 2");
  HirContext ctx{diagnostics, sourceId};
  const auto operands = operandsOf(root);

  const auto *lhsHir =
      ctx.getBuilder().makeIntLit(1);
  const auto *rhsHir =
      ctx.getBuilder().makeIntLit(2);
  ctx.getSourceTable().bind(lhsHir->getId(), operands.lhs);
  ctx.getSourceTable().bind(rhsHir->getId(), operands.rhs);

  const std::array<const HirNode *, 2> pair = {lhsHir, rhsHir};
  const auto span = ctx.spanFor(llvm::ArrayRef<const HirNode *>{pair});
  EXPECT_EQ(span.source, sourceId);
  EXPECT_EQ(span.start, 0u);
  EXPECT_EQ(span.end, 5u);
}

TEST_F(HirContextTest, SpanForArrayOfExprPointersInstantiatesTemplate) {
  // `spanFor` is templated on the element type so that
  // `ArrayRef<const Expr *>` (the shape op-resolves carry) works
  // without a copy. This test pins the template instantiation.
  const auto [root, sourceId] = parse(U"1 + 2");
  HirContext ctx{diagnostics, sourceId};
  const auto operands = operandsOf(root);

  const Expr *lhsHir =
      ctx.getBuilder().makeIntLit(1);
  const Expr *rhsHir =
      ctx.getBuilder().makeIntLit(2);
  ctx.getSourceTable().bind(lhsHir->getId(), operands.lhs);
  ctx.getSourceTable().bind(rhsHir->getId(), operands.rhs);

  const std::array<const Expr *, 2> pair = {lhsHir, rhsHir};
  const auto span = ctx.spanFor(llvm::ArrayRef<const Expr *>{pair});
  EXPECT_EQ(span.start, 0u);
  EXPECT_EQ(span.end, 5u);
}

//===----------------------------------------------------------------------===//
// error() integration — confirms the diagnostic carries the right span,
// not just that `spanFor` itself is correct.
//===----------------------------------------------------------------------===//

TEST_F(HirContextTest, ErrorOnNodeRecordsDiagnosticAtNodeSpan) {
  const auto [root, sourceId] = parse(U"1 + 2");
  HirContext ctx{diagnostics, sourceId};
  const auto operands = operandsOf(root);
  const auto *lit =
      ctx.getBuilder().makeIntLit(0);
  ctx.getSourceTable().bind(lit->getId(), operands.lhs);

  ctx.error(lit, "something is wrong").emit();

  ASSERT_EQ(diagnostics.getErrorCount(), 1u);
  const auto &diag = diagnostics.getDiagnostics()[0];
  ASSERT_FALSE(diag.labels.empty());
  const auto &span = diag.labels[0].span;
  EXPECT_EQ(span.source, sourceId);
  EXPECT_EQ(span.start, 0u);
  EXPECT_EQ(span.end, 1u);
  EXPECT_EQ(diag.message, "something is wrong");
}

TEST_F(HirContextTest, ErrorOnArrayRecordsStitchedSpan) {
  const auto [root, sourceId] = parse(U"1 + 2");
  HirContext ctx{diagnostics, sourceId};
  const auto operands = operandsOf(root);

  const Expr *lhsHir =
      ctx.getBuilder().makeIntLit(1);
  const Expr *rhsHir =
      ctx.getBuilder().makeIntLit(2);
  ctx.getSourceTable().bind(lhsHir->getId(), operands.lhs);
  ctx.getSourceTable().bind(rhsHir->getId(), operands.rhs);

  const std::array<const Expr *, 2> pair = {lhsHir, rhsHir};
  ctx.error(llvm::ArrayRef<const Expr *>{pair}, "cannot apply").emit();

  ASSERT_EQ(diagnostics.getErrorCount(), 1u);
  const auto &diag = diagnostics.getDiagnostics()[0];
  ASSERT_FALSE(diag.labels.empty());
  const auto &span = diag.labels[0].span;
  EXPECT_EQ(span.start, 0u);
  EXPECT_EQ(span.end, 5u);
}

} // namespace
