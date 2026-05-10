#include "yuzu/Syntax/Green/GreenPrinter.h"

#include "yuzu/Syntax/Green/Green.h"

#include <gtest/gtest.h>

#include <vector>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenPrinter;
using yuzu::syntax::GreenToken;

TEST(GreenPrinterTest, PrintsLoneToken) {
  const auto token = GreenToken(2, U"3");
  EXPECT_EQ(R"(Token 2@0..1 "3")", GreenPrinter::printToString(token));
}

TEST(GreenPrinterTest, PrintsFlatNode) {
  const auto node = GreenNode::create(
      12, std::vector<GreenElement>{GreenToken(2, U"3"), GreenToken(3, U"-"),
                                    GreenToken(2, U"2")});

  EXPECT_EQ(R"(Node 12@0..3
  Token 2@0..1 "3"
  Token 3@1..2 "-"
  Token 2@2..3 "2")",
            GreenPrinter::printToString(node));
}

TEST(GreenPrinterTest, PrintsNestedNodeWithOffsets) {
  const auto leftNode = GreenNode::create(
      12, std::vector<GreenElement>{GreenToken(2, U"3"), GreenToken(3, U"-"),
                                    GreenToken(2, U"2")});
  const auto equalToken = GreenToken(9, U"=");
  const auto rightNode = GreenNode::create(
      11, std::vector<GreenElement>{GreenToken(2, U"4"), GreenToken(3, U"+"),
                                    GreenToken(2, U"7")});
  const auto root = GreenNode::create(
      19, std::vector<GreenElement>{leftNode, equalToken, rightNode});

  EXPECT_EQ(R"(Node 19@0..7
  Node 12@0..3
    Token 2@0..1 "3"
    Token 3@1..2 "-"
    Token 2@2..3 "2"
  Token 9@3..4 "="
  Node 11@4..7
    Token 2@4..5 "4"
    Token 3@5..6 "+"
    Token 2@6..7 "7")",
            GreenPrinter::printToString(root));
}

TEST(GreenPrinterTest, PrintsEmptyNode) {
  const auto node = GreenNode::create(7, std::vector<GreenElement>{});
  EXPECT_EQ("Node 7@0..0", GreenPrinter::printToString(node));
}

TEST(GreenPrinterTest, EscapesSpecialCharactersInTokenText) {
  const auto token = GreenToken(2, U"a\\b\"c\nd\re\tf");
  EXPECT_EQ(R"(Token 2@0..11 "a\\b\"c\nd\re\tf")",
            GreenPrinter::printToString(token));
}

TEST(GreenPrinterTest, EncodesNonAsciiAsUtf8) {
  // U"é" is e-acute (2 bytes in UTF-8: 0xC3 0xA9).
  const auto token = GreenToken(2, U"é");
  EXPECT_EQ("Token 2@0..1 \"\xC3\xA9\"", GreenPrinter::printToString(token));
}

TEST(GreenPrinterTest, EncodesAstralCodePointAsUtf8) {
  // U+1F600 (grinning face) encodes to 4 bytes in UTF-8.
  const auto token = GreenToken(2, U"\U0001F600");
  EXPECT_EQ("Token 2@0..1 \"\xF0\x9F\x98\x80\"",
            GreenPrinter::printToString(token));
}

TEST(GreenPrinterTest, PrintsElementHoldingNode) {
  const auto node =
      GreenNode::create(12, std::vector<GreenElement>{GreenToken(2, U"3")});
  const GreenElement element = node;

  EXPECT_EQ(R"(Node 12@0..1
  Token 2@0..1 "3")",
            GreenPrinter::printToString(element));
}

TEST(GreenPrinterTest, PrintsElementHoldingToken) {
  const auto token = GreenToken(5, U"x");
  const GreenElement element = token;

  EXPECT_EQ(R"(Token 5@0..1 "x")", GreenPrinter::printToString(element));
}

} // namespace
