#include "yuzu/Ast/Expr.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Ast/AstIterator.h"
#include "yuzu/Ast/Syntax.h"
#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Syntax.h"

#include "gtest/gtest.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace {
using yuzu::ast::AstNode;
using yuzu::ast::BinaryExpr;
using yuzu::ast::Expr;
using yuzu::ast::LiteralExpr;
using yuzu::ast::SyntaxKind;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::SyntaxNode;

TEST(ExprTest, CastNoChildren) {
  SyntaxNode syntax = SyntaxNode::createRoot(
      GreenNode::create(static_cast<uint16_t>(SyntaxKind::InfixExpr),
                        std::vector<GreenElement>()));

  const std::optional<BinaryExpr> CastedNode = BinaryExpr::cast(syntax);
  EXPECT_TRUE(CastedNode.has_value());
}
} // namespace
