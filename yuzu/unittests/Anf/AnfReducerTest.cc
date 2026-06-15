#include "yuzu/Anf/Reduction/AnfReducer.h"

#include "yuzu/Anf/Anf.h"
#include "yuzu/Anf/AnfContext.h"
#include "yuzu/Anf/AnfLowerer.h"
#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Hir/HirContext.h"
#include "yuzu/Hir/HirLowerer.h"
#include "yuzu/Hir/Types/TypeResolver.h"
#include "yuzu/Lexer/Lexer.h"
#include "yuzu/Parser/Grammar/Grammar.h"
#include "yuzu/Parser/Parser.h"
#include "yuzu/Parser/TokenSink.h"
#include "yuzu/Parser/TokenSource.h"

#include <gtest/gtest.h>

namespace {

using namespace yuzu;

// Compiles a source string to typed HIR, lowers it to ANF, and reduces it, so
// the reducer can be exercised against real programs.
class AnfReducerTest : public ::testing::Test {
protected:
  diagnostics::SourceMap sources;
  diagnostics::DiagnosticsEngine diagnostics;
  diagnostics::SourceId sourceId = sources.add("<test>", U"");
  hir::HirContext hirCtx{diagnostics, sourceId};

  const hir::Root *compile(std::u32string_view source) {
    auto tokens = lexer::Lexer(source).getTokens();
    auto parser = parser::Parser(parser::TokenSource(tokens));
    parser::parseRoot(parser);
    auto events = std::move(parser).finish();
    auto sink = parser::TokenSink(std::move(tokens), std::move(events),
                                  diagnostics, sourceId);
    auto sinkResult = sink.finish();
    const auto syntaxRoot = ast::SyntaxNode::createRoot(sinkResult.green);

    const hir::Root *root = hir::HirLowerer(hirCtx, diagnostics, sourceId)
                                .lower(ast::Root{syntaxRoot});
    hir::TypeResolver(hirCtx).resolve(root);
    return root;
  }

  // The binding named `name` declared directly in `block`, or null.
  static const anf::Binding *findBinding(const anf::BlockStmt *block,
                                         std::u32string_view name) {
    for (const anf::Stmt *stmt : block->getStmts()) {
      if (const auto *let = anf::LetStmt::cast(stmt)) {
        if (let->getBinding()->getIdent()->getName() == name) {
          return let->getBinding();
        }
      }
    }
    return nullptr;
  }
};

// `a + b` over two constant bindings folds to a single constant.
TEST_F(AnfReducerTest, FoldsConstantArithmetic) {
  const hir::Root *root = compile(UR"(
    fn f() -> int32 {
      let a = 2
      let b = 3
      let c = a + b
      return c
    }
  )");
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};
  anf::AnfLowerer lowerer{anfCtx, hirCtx, diagnostics, sourceId};
  const anf::Root *program = lowerer.lowerRoot(root);
  anf::AnfReducer(anfCtx).reduce(program);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  const auto *func = anf::FuncStmt::cast(program->getStmts().front());
  ASSERT_NE(func, nullptr);
  const auto *c = findBinding(func->getBody(), U"c");
  ASSERT_NE(c, nullptr);
  const auto *folded = anf::IntConst::cast(c->getValue());
  ASSERT_NE(folded, nullptr);
  EXPECT_EQ(folded->getValue(), 5);
}

// Folding cascades: each binding resolves through the constants the previous
// ones folded to, so `(2 + 3) * 2` collapses in one walk.
TEST_F(AnfReducerTest, FoldsChainedArithmetic) {
  const hir::Root *root = compile(UR"(
    fn f() -> int32 {
      let a = 2
      let b = a + 3
      let c = b * 2
      return c
    }
  )");
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};
  anf::AnfLowerer lowerer{anfCtx, hirCtx, diagnostics, sourceId};
  const anf::Root *program = lowerer.lowerRoot(root);
  anf::AnfReducer(anfCtx).reduce(program);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  const auto *func = anf::FuncStmt::cast(program->getStmts().front());
  ASSERT_NE(func, nullptr);
  const auto *b = findBinding(func->getBody(), U"b");
  ASSERT_NE(b, nullptr);
  const auto *foldedB = anf::IntConst::cast(b->getValue());
  ASSERT_NE(foldedB, nullptr);
  EXPECT_EQ(foldedB->getValue(), 5);

  const auto *c = findBinding(func->getBody(), U"c");
  ASSERT_NE(c, nullptr);
  const auto *foldedC = anf::IntConst::cast(c->getValue());
  ASSERT_NE(foldedC, nullptr);
  EXPECT_EQ(foldedC->getValue(), 10);
}

// Folding a multiplication that overflows int64 warns and leaves the call
// unfolded rather than baking in a wrapped value.
TEST_F(AnfReducerTest, WarnsOnOverflow) {
  const hir::Root *root = compile(UR"(
    fn f() -> int64 {
      let a = 9223372036854775807
      let b = 2
      let c = a * b
      return c
    }
  )");
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};
  anf::AnfLowerer lowerer{anfCtx, hirCtx, diagnostics, sourceId};
  const anf::Root *program = lowerer.lowerRoot(root);
  anf::AnfReducer(anfCtx).reduce(program);

  EXPECT_EQ(diagnostics.getWarningCount(), 1u);

  const auto *func = anf::FuncStmt::cast(program->getStmts().front());
  ASSERT_NE(func, nullptr);
  const auto *c = findBinding(func->getBody(), U"c");
  ASSERT_NE(c, nullptr);
  // Not folded: the value stays a call, not a constant.
  EXPECT_EQ(anf::IntConst::cast(c->getValue()), nullptr);
  EXPECT_NE(anf::CallExpr::cast(c->getValue()), nullptr);
}

// A direct call in a query column is inlined: the callee's body is spliced in
// (param substituted by the argument), leaving no `FuncCallExpr` behind.
TEST_F(AnfReducerTest, InlinesDirectCallInQuery) {
  const hir::Root *root = compile(UR"(
    struct Employee { id: int32, salary: int32 }
    table employees = Employee
    fn bonus(s: int32) -> int32 {
      return s * 2
    }
    from employees e |> select bonus(e.salary) as x
  )");
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};
  anf::AnfLowerer lowerer{anfCtx, hirCtx, diagnostics, sourceId};
  const anf::Root *program = lowerer.lowerRoot(root);
  anf::AnfReducer(anfCtx).reduce(program);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  const auto *exprStmt = anf::ExprStmt::cast(program->getStmts().back());
  ASSERT_NE(exprStmt, nullptr);
  const auto *select = anf::SelectRel::cast(exprStmt->getValue());
  ASSERT_NE(select, nullptr);
  ASSERT_EQ(select->getItems().size(), 1u);
  const auto *body = select->getItems().front()->getBody();

  // No call survives, and the multiply reads `e.salary` directly (param
  // substituted by the argument atom).
  bool sawMulOverField = false;
  for (const anf::Stmt *stmt : body->getStmts()) {
    const auto *let = anf::LetStmt::cast(stmt);
    if (let == nullptr) {
      continue;
    }
    const anf::Expr *value = let->getBinding()->getValue();
    EXPECT_EQ(anf::FuncCallExpr::cast(value), nullptr);
    if (const auto *call = anf::CallExpr::cast(value)) {
      if (!call->getArgs().empty() &&
          anf::FieldAtom::cast(call->getArgs().front()) != nullptr) {
        sawMulOverField = true;
      }
    }
  }
  EXPECT_TRUE(sawMulOverField);
}

} // namespace
