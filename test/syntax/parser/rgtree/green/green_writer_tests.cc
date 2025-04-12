#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "syntax/parser/rgtree/green/green_element.h"
#include "syntax/parser/rgtree/green/green_writer.h"
#include "syntax/parser/rgtree/green/green_node.h"
#include "syntax/parser/rgtree/green/green_token.h"
#include "syntax/syntax_kind.h"

namespace {
using orion::syntax::GreenElement;
using orion::syntax::GreenWriter;
using orion::syntax::GreenNode;
using orion::syntax::GreenToken;
using orion::syntax::SyntaxKind;

const auto kGreenTokenPlus = GreenToken(SyntaxKind::kPlus, U"+");
const auto kGreenTokenMinus = GreenToken(SyntaxKind::kMinus, U"-");
const auto kGreenToken1 = GreenToken(SyntaxKind::kIntLiteral, U"1");
const auto kGreenToken2 = GreenToken(SyntaxKind::kIntLiteral, U"2");
const auto kGreenToken3 = GreenToken(SyntaxKind::kIntLiteral, U"3");

TEST(GreenElementWriterTests, GreenToken) {
  const auto actual = GreenWriter::WriteAsU32String(kGreenToken1);
  const auto expected = U"IntLiteral@0..1 \"1\"\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, GreenNodeNoChildren) {
  const auto node = GreenNode(SyntaxKind::kRoot, std::vector<GreenElement>{});
  const auto actual = GreenWriter::WriteAsU32String(node);
  const auto expected = U"Root@0..0\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, GreenNodeWithTokenChildren) {
  const auto node = GreenNode(
      SyntaxKind::kBinaryExpr,
      std::vector{GreenElement(kGreenToken1), GreenElement(kGreenTokenPlus),
                  GreenElement(kGreenToken2)});
  const auto actual = GreenWriter::WriteAsU32String(node);
  const auto expected =
      U"BinaryExpr@0..3\n"
      U"  IntLiteral@0..1 \"1\"\n"
      U"  Plus@1..2 \"+\"\n"
      U"  IntLiteral@2..3 \"2\"\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, GreenNodeWithTokenChildrenAndLargerIndent) {
  const auto node = GreenNode(
      SyntaxKind::kBinaryExpr,
      std::vector{GreenElement(kGreenToken1), GreenElement(kGreenTokenPlus),
                  GreenElement(kGreenToken2)});
  const auto actual = GreenWriter::WriteAsU32String(node, 4);
  const auto expected =
      U"BinaryExpr@0..3\n"
      U"    IntLiteral@0..1 \"1\"\n"
      U"    Plus@1..2 \"+\"\n"
      U"    IntLiteral@2..3 \"2\"\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, GreenNodeWithTokenAndNodeChildren) {
  const auto node = GreenNode(
      SyntaxKind::kBinaryExpr,
      std::vector{
          GreenElement(kGreenToken1), GreenElement(kGreenTokenPlus),
          GreenElement(GreenNode(SyntaxKind::kBinaryExpr,
                                 std::vector{GreenElement(kGreenToken2),
                                             GreenElement(kGreenTokenMinus),
                                             GreenElement(kGreenToken3)}))});
  const auto actual = GreenWriter::WriteAsU32String(node);
  const auto expected =
      U"BinaryExpr@0..5\n"
      U"  IntLiteral@0..1 \"1\"\n"
      U"  Plus@1..2 \"+\"\n"
      U"  BinaryExpr@2..5\n"
      U"    IntLiteral@2..3 \"2\"\n"
      U"    Minus@3..4 \"-\"\n"
      U"    IntLiteral@4..5 \"3\"\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, GreenNodeWithTokenAndNodeChildrenReverseOrder) {
  const auto node = GreenNode(
      SyntaxKind::kBinaryExpr,
      std::vector{
          GreenElement(GreenNode(SyntaxKind::kBinaryExpr,
                                 std::vector{GreenElement(kGreenToken2),
                                             GreenElement(kGreenTokenMinus),
                                             GreenElement(kGreenToken3)})),
          GreenElement(kGreenTokenPlus),
          GreenElement(kGreenToken1),
      });
  const auto actual = GreenWriter::WriteAsU32String(node);
  const auto expected =
      U"BinaryExpr@0..5\n"
      U"  BinaryExpr@0..3\n"
      U"    IntLiteral@0..1 \"2\"\n"
      U"    Minus@1..2 \"-\"\n"
      U"    IntLiteral@2..3 \"3\"\n"
      U"  Plus@3..4 \"+\"\n"
      U"  IntLiteral@4..5 \"1\"\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, Clear) {
  auto writer = GreenWriter();
  writer.Write(kGreenToken1).Clear();

  const auto actual = writer.AsU32String();
  const auto expected = U"";
  EXPECT_EQ(expected, actual);
}
};  // namespace
