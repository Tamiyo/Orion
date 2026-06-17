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

// Compiles a source string to a typed HIR Root (lex -> parse -> lower -> type)
// so the lowerer can be driven against real HIR.
class AnfLowererTest : public ::testing::Test {
protected:
  diagnostics::SourceMap sources;
  diagnostics::DiagnosticsEngine diagnostics;
  diagnostics::SourceId sourceId = sources.add("<test>", U"");
  util::StringInterner interner;
  hir::HirContext hirCtx{diagnostics, sourceId, interner};

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
};

// A scalar function body lowers to a `BlockStmt` of `let`s ending in a
// `ReturnStmt`. The computed return value `y * 2` is forced into a `%t`
// temporary because `return` takes an `Atom`.
TEST_F(AnfLowererTest, LowersScalarFunctionBody) {
  const hir::Root *root = compile(UR"(
    fn double(x: int32) -> int32 {
      let y = x + 1
      return y * 2
    }
  )");
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx, interner};
  anf::AnfLowerer lowerer{anfCtx, diagnostics, sourceId};
  const anf::Root *program = lowerer.lowerRoot(root);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  ASSERT_EQ(program->getStmts().size(), 1u);
  const auto *func = anf::FuncStmt::cast(program->getStmts().front());
  ASSERT_NE(func, nullptr);
  ASSERT_EQ(func->getParams().size(), 1u);

  // body: let y = x+1 ; let %t = y*2 ; return %t
  const auto *body = func->getBody();
  ASSERT_NE(body, nullptr);
  ASSERT_EQ(body->getStmts().size(), 3u);
  EXPECT_NE(anf::LetStmt::cast(body->getStmts()[0]), nullptr);
  EXPECT_NE(anf::LetStmt::cast(body->getStmts()[1]), nullptr);
  const auto *ret = anf::ReturnStmt::cast(body->getStmts()[2]);
  ASSERT_NE(ret, nullptr);
  EXPECT_NE(anf::VarAtom::cast(ret->getValue()), nullptr);
}

// A query lowers to `ExprStmt(SelectRel(FromRel, [SelectItem]))`. The column
// `e.salary * 2` is a per-row block whose tail is the multiply, and `e.salary`
// is a `FieldAtom` over the row binding `from` synthesized.
TEST_F(AnfLowererTest, LowersQuery) {
  const hir::Root *root = compile(UR"(
    struct Employee { id: int32, salary: int32 }
    table employees = Employee
    from employees e |> select e.salary * 2 as x
  )");
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx, interner};
  anf::AnfLowerer lowerer{anfCtx, diagnostics, sourceId};
  const anf::Root *program = lowerer.lowerRoot(root);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  // struct decl, table decl, then the query as an ExprStmt.
  ASSERT_EQ(program->getStmts().size(), 3u);
  const auto *exprStmt = anf::ExprStmt::cast(program->getStmts().back());
  ASSERT_NE(exprStmt, nullptr);
  const auto *select = anf::SelectRel::cast(exprStmt->getValue());
  ASSERT_NE(select, nullptr);
  EXPECT_NE(anf::FromRel::cast(select->getInput()), nullptr);

  ASSERT_EQ(select->getItems().size(), 1u);
  const auto *body = select->getItems().front()->getBody();
  ASSERT_NE(body, nullptr);
  // The column body resolves to the multiply at its tail.
  const auto *tail = anf::ExprStmt::cast(body->getStmts().back());
  ASSERT_NE(tail, nullptr);
  const auto *mul = anf::CallExpr::cast(tail->getValue());
  ASSERT_NE(mul, nullptr);
  // First operand is `e.salary` — a FieldAtom over the row's VarAtom.
  ASSERT_FALSE(mul->getArgs().empty());
  const auto *field = anf::FieldAtom::cast(mul->getArgs().front());
  ASSERT_NE(field, nullptr);
  EXPECT_NE(anf::VarAtom::cast(field->getBase()), nullptr);
}

// A direct call `inc(7)` lowers its callee to a `FuncRef` pointing at the
// callee function's lowered `FuncStmt`, so a later inlining pass can chase it.
TEST_F(AnfLowererTest, DirectCallLowersToFuncRef) {
  const hir::Root *root = compile(UR"(
    fn inc(x: int32) -> int32 {
      return x + 1
    }
    fn main() -> int32 {
      return inc(7)
    }
  )");
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  anf::AnfContext anfCtx{diagnostics, sourceId, hirCtx, interner};
  anf::AnfLowerer lowerer{anfCtx, diagnostics, sourceId};
  const anf::Root *program = lowerer.lowerRoot(root);
  ASSERT_TRUE(diagnostics.getDiagnostics().empty());

  ASSERT_EQ(program->getStmts().size(), 2u);
  const auto *inc = anf::FuncStmt::cast(program->getStmts()[0]);
  const auto *main = anf::FuncStmt::cast(program->getStmts()[1]);
  ASSERT_NE(inc, nullptr);
  ASSERT_NE(main, nullptr);

  // main's body: let %t = inc(7) ; return %t  (the call is forced to a temp).
  const auto *body = main->getBody();
  ASSERT_FALSE(body->getStmts().empty());
  const auto *let = anf::LetStmt::cast(body->getStmts().front());
  ASSERT_NE(let, nullptr);
  const auto *call = anf::FuncCallExpr::cast(let->getBinding()->getValue());
  ASSERT_NE(call, nullptr);

  // The callee resolves to a FuncRef whose target is `inc`.
  const auto *funcRef = anf::FuncRef::cast(call->getCallee());
  ASSERT_NE(funcRef, nullptr);
  EXPECT_EQ(funcRef->getFunc(), inc);
}

} // namespace
