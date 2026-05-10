#include "yuzu/Syntax/SyntaxPrinter.h"

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Syntax.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace test::ast {
/// Standalone Kind enum standing in for a frontend-generated SyntaxKind. The
/// printer needs `asString(Kind)` reachable by ADL from this namespace.
enum class TestKind : uint16_t {
  Root = 1,
  Inner = 2,
  Plus = 3,
  Number = 4,
  Other = 5,
};

inline std::string asString(TestKind kind) {
  switch (kind) {
  case TestKind::Root:
    return "Root";
  case TestKind::Inner:
    return "Inner";
  case TestKind::Plus:
    return "Plus";
  case TestKind::Number:
    return "Number";
  case TestKind::Other:
    return "Other";
  }
  return "Unknown";
}
} // namespace test::ast

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;

using TestKind = test::ast::TestKind;
using SyntaxElement = yuzu::syntax::SyntaxElement<TestKind>;
using SyntaxNode = yuzu::syntax::SyntaxNode<TestKind>;
using SyntaxPrinter = yuzu::syntax::SyntaxPrinter<TestKind>;
using SyntaxToken = yuzu::syntax::SyntaxToken<TestKind>;

uint16_t raw(TestKind k) { return static_cast<uint16_t>(k); }

GreenToken token(TestKind k, std::u32string_view text) {
  return GreenToken(raw(k), std::u32string(text));
}

GreenNode node(TestKind k, std::vector<GreenElement> children) {
  return GreenNode::create(raw(k), std::move(children));
}

SyntaxToken looseToken(TestKind k, std::u32string_view text) {
  return SyntaxToken(yuzu::syntax::detail::SyntaxToken(0, 0, token(k, text)));
}

TEST(SyntaxPrinterTest, PrintsLoneToken) {
  EXPECT_EQ(R"(Number@0..1 "3")",
            SyntaxPrinter::printToString(looseToken(TestKind::Number, U"3")));
}

TEST(SyntaxPrinterTest, PrintsFlatNode) {
  const auto green = node(TestKind::Root, {token(TestKind::Number, U"3"),
                                           token(TestKind::Plus, U"-"),
                                           token(TestKind::Number, U"2")});
  const auto root = SyntaxNode::createRoot(green);

  EXPECT_EQ(R"(Root@0..3
  Number@0..1 "3"
  Plus@1..2 "-"
  Number@2..3 "2")",
            SyntaxPrinter::printToString(root));
}

TEST(SyntaxPrinterTest, PrintsNestedNodeWithOffsets) {
  const auto leftNode = node(TestKind::Inner, {token(TestKind::Number, U"3"),
                                               token(TestKind::Plus, U"-"),
                                               token(TestKind::Number, U"2")});
  const auto rightNode = node(TestKind::Inner, {token(TestKind::Number, U"4"),
                                                token(TestKind::Plus, U"+"),
                                                token(TestKind::Number, U"7")});
  const auto green = node(
      TestKind::Root, {leftNode, token(TestKind::Other, U"="), rightNode});
  const auto root = SyntaxNode::createRoot(green);

  EXPECT_EQ(R"(Root@0..7
  Inner@0..3
    Number@0..1 "3"
    Plus@1..2 "-"
    Number@2..3 "2"
  Other@3..4 "="
  Inner@4..7
    Number@4..5 "4"
    Plus@5..6 "+"
    Number@6..7 "7")",
            SyntaxPrinter::printToString(root));
}

TEST(SyntaxPrinterTest, PrintsEmptyNode) {
  const auto root = SyntaxNode::createRoot(node(TestKind::Root, {}));
  EXPECT_EQ("Root@0..0", SyntaxPrinter::printToString(root));
}

TEST(SyntaxPrinterTest, EscapesSpecialCharactersInTokenText) {
  EXPECT_EQ(R"(Other@0..11 "a\\b\"c\nd\re\tf")",
            SyntaxPrinter::printToString(
                looseToken(TestKind::Other, U"a\\b\"c\nd\re\tf")));
}

TEST(SyntaxPrinterTest, EncodesNonAsciiAsUtf8) {
  // U"é" is e-acute (2 bytes in UTF-8: 0xC3 0xA9).
  EXPECT_EQ("Other@0..1 \"\xC3\xA9\"",
            SyntaxPrinter::printToString(looseToken(TestKind::Other, U"é")));
}

TEST(SyntaxPrinterTest, EncodesAstralCodePointAsUtf8) {
  // U+1F600 (grinning face) encodes to 4 bytes in UTF-8.
  EXPECT_EQ(
      "Other@0..1 \"\xF0\x9F\x98\x80\"",
      SyntaxPrinter::printToString(looseToken(TestKind::Other, U"\U0001F600")));
}

TEST(SyntaxPrinterTest, PrintsElementHoldingNode) {
  const auto green = node(TestKind::Root, {token(TestKind::Number, U"3")});
  const auto root = SyntaxNode::createRoot(green);
  const SyntaxElement element = root;

  EXPECT_EQ(R"(Root@0..1
  Number@0..1 "3")",
            SyntaxPrinter::printToString(element));
}

TEST(SyntaxPrinterTest, PrintsElementHoldingToken) {
  const SyntaxElement element = looseToken(TestKind::Other, U"x");

  EXPECT_EQ(R"(Other@0..1 "x")", SyntaxPrinter::printToString(element));
}

TEST(SyntaxPrinterTest, OffsetsReflectChildPosition) {
  // A subtree printed from a non-root node should still show its absolute
  // offsets, not zero-based ones.
  const auto inner = node(TestKind::Inner, {token(TestKind::Number, U"4"),
                                            token(TestKind::Plus, U"+"),
                                            token(TestKind::Number, U"7")});
  const auto green =
      node(TestKind::Root, {token(TestKind::Number, U"3"),
                            token(TestKind::Other, U"="), inner});
  const auto root = SyntaxNode::createRoot(green);

  // Walk to the inner node (third child).
  auto it = root.getChildrenWithTokens().begin();
  ++it;
  ++it;
  const SyntaxNode innerSyntax = (*it).getNode();

  EXPECT_EQ(R"(Inner@2..5
  Number@2..3 "4"
  Plus@3..4 "+"
  Number@4..5 "7")",
            SyntaxPrinter::printToString(innerSyntax));
}

} // namespace
