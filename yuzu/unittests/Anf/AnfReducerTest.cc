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

// Compiles a source string to typed HIR, lowers it to ANF, and reduces it.
// Reduction is rooted at the query, so the tests drive it through `select`
// columns.
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

  // Lower `source` to ANF and reduce it; return the reduced program.
  const anf::Root *lowerAndReduce(std::u32string_view source,
                                  anf::AnfContext &anfCtx) {
    const hir::Root *root = compile(source);
    EXPECT_TRUE(diagnostics.getDiagnostics().empty());
    anf::AnfLowerer lowerer{anfCtx, hirCtx, diagnostics, sourceId};
    const anf::Root *program = lowerer.lowerRoot(root);
    anf::AnfReducer(anfCtx).reduce(program);
    return program;
  }

  // The body (a `Thunk`) of the query's first `select` column.
  static const anf::Thunk *firstColumn(const anf::Root *program) {
    const auto *exprStmt = anf::ExprStmt::cast(program->getStmts().back());
    EXPECT_NE(exprStmt, nullptr);
    const auto *select = anf::SelectRel::cast(exprStmt->getValue());
    EXPECT_NE(select, nullptr);
    return select->getItems().front()->getBody();
  }

  // The atom a column thunk yields (its tail `ExprStmt`'s value).
  static const anf::Atom *columnTail(const anf::Thunk *body) {
    const auto *tail = anf::ExprStmt::cast(body->getStmts().back());
    EXPECT_NE(tail, nullptr);
    return anf::Atom::cast(tail->getValue());
  }
};

// A constant column expression folds away entirely: `2 + 3` becomes `5`.
TEST_F(AnfReducerTest, FoldsConstantArithmetic) {
  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};
  const anf::Root *program = lowerAndReduce(UR"(
    struct S { v: int32 }
    table t = S
    from t e |> select 2 + 3 as x
  )",
                                            anfCtx);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  const auto *folded = anf::IntConst::cast(columnTail(firstColumn(program)));
  ASSERT_NE(folded, nullptr);
  EXPECT_EQ(folded->getValue(), 5);
}

// Folding flows through nested constants in one pass: `(2 + 3) * 2` -> `10`.
TEST_F(AnfReducerTest, FoldsChainedArithmetic) {
  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};
  const anf::Root *program = lowerAndReduce(UR"(
    struct S { v: int32 }
    table t = S
    from t e |> select (2 + 3) * 2 as x
  )",
                                            anfCtx);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  const auto *folded = anf::IntConst::cast(columnTail(firstColumn(program)));
  ASSERT_NE(folded, nullptr);
  EXPECT_EQ(folded->getValue(), 10);
}

// Folding a multiplication that overflows int64 warns and leaves the call in
// place rather than baking in a wrapped value.
TEST_F(AnfReducerTest, WarnsOnOverflow) {
  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};
  const anf::Root *program = lowerAndReduce(UR"(
    struct S { v: int32 }
    table t = S
    from t e |> select 9223372036854775807 * 2 as x
  )",
                                            anfCtx);

  EXPECT_EQ(diagnostics.getWarningCount(), 1u);

  // Not folded: a multiply survives as a binding in the column.
  bool sawCall = false;
  for (const anf::Stmt *stmt : firstColumn(program)->getStmts()) {
    if (const auto *let = anf::LetStmt::cast(stmt)) {
      if (anf::CallExpr::cast(let->getBinding()->getValue()) != nullptr) {
        sawCall = true;
      }
    }
  }
  EXPECT_TRUE(sawCall);
}

// A direct call in a column is inlined and reduced: the callee body is spliced
// in (parameter substituted by the argument), leaving no `FuncCallExpr` and the
// multiply reading `e.salary` directly.
TEST_F(AnfReducerTest, InlinesDirectCallInQuery) {
  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};
  const anf::Root *program = lowerAndReduce(UR"(
    struct Employee { id: int32, salary: int32 }
    table employees = Employee
    fn bonus(s: int32) -> int32 {
      return s * 2
    }
    from employees e |> select bonus(e.salary) as x
  )",
                                            anfCtx);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  const anf::Thunk *body = firstColumn(program);
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

// A function fully inlined into the query has no callers left, so it is dropped
// from the program as dead code.
TEST_F(AnfReducerTest, DeletesInlinedFunction) {
  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx};
  const anf::Root *program = lowerAndReduce(UR"(
    struct Employee { id: int32, salary: int32 }
    table employees = Employee
    fn bonus(s: int32) -> int32 {
      return s * 2
    }
    from employees e |> select bonus(e.salary) as x
  )",
                                            anfCtx);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  for (const anf::Stmt *stmt : program->getStmts()) {
    EXPECT_EQ(anf::FuncStmt::cast(stmt), nullptr);
  }
}

} // namespace
