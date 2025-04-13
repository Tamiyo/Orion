#include <gtest/gtest.h>

#include <string>
#include <string_view>
#include <vector>

#include "syntax/parser/rgtree/green/green.h"
#include "syntax/parser/rgtree/green/green_writer.h"

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::GreenWriter;

enum class SyntaxKind : uint16_t {
  kPlus,
  kMinus,
  kIdentifier,
  kRoot,
  kBinaryExpr,
  kIntLiteral
};

// const std::function<std::u32string_view(SyntaxKind)>&
// syntax_kind_to_u32string_view,

std::u32string_view SyntaxKindToU32StringView(const SyntaxKind kind) {
  switch (kind) {
    case SyntaxKind::kPlus:
      return U"Plus";
    case SyntaxKind::kMinus:
      return U"Minus";
    case SyntaxKind::kIdentifier:
      return U"Identifier";
    case SyntaxKind::kRoot:
      return U"Root";
    case SyntaxKind::kBinaryExpr:
      return U"BinaryExpr";
    case SyntaxKind::kIntLiteral:
      return U"IntLiteral";
    default:
      return U"";
  }
}

const auto kGreenTokenPlus = GreenToken(SyntaxKind::kPlus, U"+");
const auto kGreenTokenMinus = GreenToken(SyntaxKind::kMinus, U"-");
const auto kGreenToken1 = GreenToken(SyntaxKind::kIdentifier, U"🍕");
const auto kGreenToken2 = GreenToken(SyntaxKind::kIntLiteral, U"2");
const auto kGreenToken3 = GreenToken(SyntaxKind::kIntLiteral, U"3");

TEST(GreenElementWriterTests, GreenToken) {
  const auto actual = GreenWriter<SyntaxKind>::WriteAsU32String(
      kGreenToken1, SyntaxKindToU32StringView);
  const auto expected = U"Identifier@0..1 \"🍕\"\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, GreenNodeNoChildren) {
  const auto node = GreenNode<SyntaxKind>(
      SyntaxKind::kRoot, std::vector<GreenElement<SyntaxKind>>{});
  const auto actual = GreenWriter<SyntaxKind>::WriteAsU32String(
      node, SyntaxKindToU32StringView);
  const auto expected = U"Root@0..0\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, GreenNodeWithTokenChildren) {
  const auto node = GreenNode<SyntaxKind>(
      SyntaxKind::kBinaryExpr,
      std::vector{GreenElement(kGreenToken1), GreenElement(kGreenTokenPlus),
                  GreenElement(kGreenToken2)});
  const auto actual = GreenWriter<SyntaxKind>::WriteAsU32String(
      node, SyntaxKindToU32StringView);
  const auto expected =
      U"BinaryExpr@0..3\n"
      U"  Identifier@0..1 \"🍕\"\n"
      U"  Plus@1..2 \"+\"\n"
      U"  IntLiteral@2..3 \"2\"\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, GreenNodeWithTokenChildrenAndLargerIndent) {
  const auto node = GreenNode(
      SyntaxKind::kBinaryExpr,
      std::vector{GreenElement(kGreenToken1), GreenElement(kGreenTokenPlus),
                  GreenElement(kGreenToken2)});
  const auto actual = GreenWriter<SyntaxKind>::WriteAsU32String(
      node, SyntaxKindToU32StringView, 4);
  const auto expected =
      U"BinaryExpr@0..3\n"
      U"    Identifier@0..1 \"🍕\"\n"
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
  const auto actual = GreenWriter<SyntaxKind>::WriteAsU32String(
      node, SyntaxKindToU32StringView);
  const auto expected =
      U"BinaryExpr@0..5\n"
      U"  Identifier@0..1 \"🍕\"\n"
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
  const auto actual = GreenWriter<SyntaxKind>::WriteAsU32String(
      node, SyntaxKindToU32StringView);
  const auto expected =
      U"BinaryExpr@0..5\n"
      U"  BinaryExpr@0..3\n"
      U"    IntLiteral@0..1 \"2\"\n"
      U"    Minus@1..2 \"-\"\n"
      U"    IntLiteral@2..3 \"3\"\n"
      U"  Plus@3..4 \"+\"\n"
      U"  Identifier@4..5 \"🍕\"\n";
  EXPECT_EQ(expected, actual);
}

TEST(GreenElementWriterTests, Clear) {
  auto writer = GreenWriter<SyntaxKind>(SyntaxKindToU32StringView);
  writer.Write(kGreenToken1).Clear();

  const auto actual = writer.AsU32String();
  const auto expected = U"";
  EXPECT_EQ(expected, actual);
}
};  // namespace
