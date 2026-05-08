#include "yuzu/Ast/Ast.h"

#include "yuzu/Syntax/Green/Green.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <optional>
#include <vector>

namespace {
using yuzu::ast::BinaryExpr;
using yuzu::ast::SyntaxKind;
using yuzu::ast::SyntaxNode;
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;

TEST(ExprTest, CastNoChildren) {
  SyntaxNode syntax = SyntaxNode::createRoot(
      GreenNode::create(static_cast<uint16_t>(SyntaxKind::BinaryExpr),
                        std::vector<GreenElement>()));

  const std::optional<BinaryExpr> CastedNode = BinaryExpr::cast(syntax);
  EXPECT_TRUE(CastedNode.has_value());
}
} // namespace
