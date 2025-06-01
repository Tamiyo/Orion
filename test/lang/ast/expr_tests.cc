#include <gtest/gtest.h>

#include <cstdio>
#include <string>
#include <vector>

#include "lang/ast/expr.h"
#include "lang/ast/syntax.h"
#include "lang/lexer/lexer.h"
#include "lang/lexer/token.h"
#include "lang/lexer/token_kind.h"
#include "lang/parser/grammar/expr.h"
#include "lang/parser/parser.h"
#include "lang/parser/syntax_kind.h"
#include "lang/parser/token_sink.h"
#include "lang/parser/token_source.h"
#include "syntax/rgtree/cursor.h"
#include "syntax/rgtree/green_writer.h"
#include "utils/strings.h"

namespace {
using yuzu::lang::Expr;
using yuzu::lang::IsTrivia;
using yuzu::lang::Lexer;
using yuzu::lang::Parser;
using yuzu::lang::SyntaxKind;
using yuzu::lang::SyntaxNode;
using yuzu::lang::Token;
using yuzu::lang::TokenKind;
using yuzu::lang::TokenSink;
using yuzu::lang::TokenSource;
using yuzu::lang::ToU32String;
using yuzu::syntax::GreenWriter;
using yuzu::utils::U32ToU8;
using SyntaxElementChildren = yuzu::syntax::SyntaxElementChildren<SyntaxKind>;

TEST(TestMe, ABC) {
  auto lexer = Lexer(U"2 + 2");
  const std::vector<Token> tokens = lexer.Tokenize();
  auto source = TokenSource(tokens, ::IsTrivia);
  auto parser = Parser(source);

  Expr(&parser);

  auto sink = TokenSink(tokens, parser.Events(), ::IsTrivia);
  const auto [node, errors] = sink.Finish();
  const SyntaxNode root = SyntaxNode::CreateRoot(node);

  const auto children = SyntaxElementChildren(root);
  for (auto it = children.Begin(); it != children.End(); ++it) {
    // const std::u32string_view kind = ToU32String((*it).Kind());
    // printf("%s", U32ToU8(std::u32string(kind)).c_str());
  }
}
}  // namespace
