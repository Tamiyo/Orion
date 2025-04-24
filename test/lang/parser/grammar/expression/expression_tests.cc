#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

#include "lang/lexer/lexer.h"
#include "lang/lexer/token.h"
#include "lang/lexer/token_kind.h"
#include "lang/parser/grammar/expression/expression.h"
#include "lang/parser/parser.h"
#include "lang/parser/syntax_kind.h"
#include "lang/parser/token_sink.h"
#include "lang/parser/token_source.h"
#include "syntax/parser/rgtree/green/green_writer.h"

namespace yuzu::lang {};

namespace {
using yuzu::lang::Expr;
using yuzu::lang::IsTrivia;
using yuzu::lang::Lexer;
using yuzu::lang::Parser;
using yuzu::lang::SyntaxKind;
using yuzu::lang::ToU32String;
using yuzu::lang::Token;
using yuzu::lang::TokenKind;
using yuzu::lang::TokenSink;
using yuzu::lang::TokenSource;

using yuzu::syntax::GreenWriter;

TEST(ParserTest, Expr) {
  auto lexer = Lexer(U"1+2");

  const std::vector<Token> tokens = lexer.Tokenize();

  auto source = TokenSource(tokens, ::IsTrivia);
  auto parser = Parser(source);

  Expr(&parser);

  auto sink = TokenSink(tokens, parser.Events());
  const auto [node, errors] = sink.Finish();

  const std::u32string actual = GreenWriter<SyntaxKind>::WriteAsU32String(
      node, ::ToU32String);

  const std::u32string expected =
      U"InfixExpr@0..3\n"
      U"  Literal@0..1\n"
      U"    IntLiteral@0..1 \"1\"\n"
      U"  Plus@1..2 \"+\"\n"
      U"  Literal@2..3\n"
      U"    IntLiteral@2..3 \"2\"\n";

  EXPECT_EQ(expected, actual);
}
};  // namespace
