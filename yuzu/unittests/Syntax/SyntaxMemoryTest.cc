#include "yuzu/Syntax/Syntax.h"

#include "gtest/gtest.h"

#include <vector>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenToken;
using yuzu::syntax::SyntaxData;
using yuzu::syntax::SyntaxNode;
using yuzu::syntax::SyntaxToken;

GreenNode createTestGreenNode() {
  return GreenNode::create(2,
                           std::vector<GreenElement>{GreenToken(1, U"test")});
}

GreenToken createTestGreenToken() { return GreenToken(1, U"test"); }

TEST(SyntaxDataTest, ConstructorInitializesRefCount) {
  const auto green = createTestGreenNode();
  const auto data = SyntaxData(green, nullptr, 0, 0);

  EXPECT_EQ(1, data.getRc());
}

TEST(SyntaxDataTest, CopyConstructorSharesRefCount) {
  const auto green = createTestGreenNode();
  const auto data1 = SyntaxData(green, nullptr, 0, 0);

  const auto data2 = SyntaxData(data1);

  EXPECT_EQ(2, data1.getRc());
  EXPECT_EQ(2, data2.getRc());
  EXPECT_EQ(data1.getRc(), data2.getRc());
}

TEST(SyntaxDataTest, CopyAssignmentHandlesRefCount) {
  const auto green1 = createTestGreenNode();
  const auto green2 = createTestGreenToken();

  auto data1 = SyntaxData(green1, nullptr, 0, 0);
  auto data2 = SyntaxData(green2, nullptr, 10, 1);

  data2 = data1;

  EXPECT_EQ(data1.getOffset(), data2.getOffset());
  EXPECT_EQ(data1.getIndex(), data2.getIndex());

  EXPECT_EQ(2, data1.getRc());
  EXPECT_EQ(2, data2.getRc());
  EXPECT_EQ(data1.getRc(), data2.getRc());
}

TEST(SyntaxDataTest, SelfAssignmentIsNoop) {
  const auto green = createTestGreenNode();
  auto data = SyntaxData(green, nullptr, 42, 7);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
  data = data;
#pragma GCC diagnostic pop

  EXPECT_EQ(42, data.getOffset());
  EXPECT_EQ(7, data.getIndex());

  EXPECT_EQ(1, data.getRc());
}

TEST(SyntaxDataTest, MoveConstructorTransfersOwnership) {
  const auto green = createTestGreenNode();
  auto data1 = SyntaxData(green, nullptr, 0, 0);

  const auto data2 = SyntaxData(std::move(data1));

  EXPECT_EQ(0, data2.getOffset());
  EXPECT_EQ(0, data2.getIndex());

  EXPECT_EQ(1, data2.getRc());
}

TEST(SyntaxDataTest, MoveAssignmentTransfersOwnership) {
  const auto green1 = createTestGreenNode();
  const auto green2 = createTestGreenToken();

  auto data1 = SyntaxData(green1, nullptr, 10, 5);
  auto data2 = SyntaxData(green2, nullptr, 20, 10);

  data2 = std::move(data1);

  EXPECT_EQ(10, data2.getOffset());
  EXPECT_EQ(5, data2.getIndex());
  EXPECT_EQ(1, data2.getRc());
}

TEST(SyntaxDataTest, GettersReturnCorrectValues) {
  const auto green = createTestGreenNode();
  SyntaxData *const parent = nullptr;
  const size_t offset = 42;
  const size_t index = 7;

  const auto data = SyntaxData(green, parent, offset, index);

  EXPECT_EQ(parent, data.getParent());
  EXPECT_EQ(offset, data.getOffset());
  EXPECT_EQ(index, data.getIndex());
}

TEST(SyntaxDataTest, MultipleReferencesShareData) {
  const auto green = createTestGreenNode();
  const auto data1 = SyntaxData(green, nullptr, 0, 0);

  const auto data2 = SyntaxData(data1);
  const auto data3 = SyntaxData(data2);
  const auto data4 = data3;

  EXPECT_EQ(data1.getOffset(), data4.getOffset());

  EXPECT_EQ(4, data1.getRc());
  EXPECT_EQ(4, data2.getRc());
  EXPECT_EQ(4, data3.getRc());
  EXPECT_EQ(4, data4.getRc());

  EXPECT_EQ(data1.getRc(), data2.getRc());
  EXPECT_EQ(data2.getRc(), data3.getRc());
  EXPECT_EQ(data3.getRc(), data4.getRc());
}

TEST(SyntaxNodeTest, ConstructorCreatesNode) {
  const auto green = createTestGreenNode();
  const auto node = SyntaxNode(10, 5, nullptr, green);

  EXPECT_EQ(10, node.getOffset());
  EXPECT_EQ(5, node.getIndex());
  EXPECT_EQ(nullptr, node.getParent());
  EXPECT_EQ(2, node.getKind());
}

TEST(SyntaxNodeTest, CreateRootCreatesRootNode) {
  const auto green = createTestGreenNode();
  const auto root = SyntaxNode::createRoot(green);

  EXPECT_EQ(0, root.getOffset());
  EXPECT_EQ(0, root.getIndex());
  EXPECT_EQ(nullptr, root.getParent());
  EXPECT_EQ(2, root.getKind());
}

TEST(SyntaxNodeTest, CopyConstructorSharesData) {
  const auto green = createTestGreenNode();
  const auto node1 = SyntaxNode(42, 7, nullptr, green);

  const auto node2 = SyntaxNode(node1);

  EXPECT_EQ(node1, node2);
  EXPECT_EQ(node1.getOffset(), node2.getOffset());
  EXPECT_EQ(node1.getIndex(), node2.getIndex());
  EXPECT_EQ(node1.getParent(), node2.getParent());
  EXPECT_EQ(node1.getKind(), node2.getKind());
}

TEST(SyntaxNodeTest, MultipleNodesCanShareData) {
  const auto green = createTestGreenNode();
  const auto node1 = SyntaxNode(100, 50, nullptr, green);

  const auto node2 = SyntaxNode(node1);
  const auto Node3 = SyntaxNode(node2);
  const auto Node4 = SyntaxNode(Node3);

  EXPECT_EQ(node1, node2);
  EXPECT_EQ(node2, Node3);
  EXPECT_EQ(Node3, Node4);
  EXPECT_EQ(node1, Node4);

  EXPECT_EQ(100, Node4.getOffset());
  EXPECT_EQ(50, Node4.getIndex());
}

TEST(SyntaxTokenTest, ConstructorWithParent) {
  const auto green = createTestGreenToken();
  const auto token = SyntaxToken(10, 5, nullptr, green);

  EXPECT_EQ(10, token.getOffset());
  EXPECT_EQ(5, token.getIndex());
  EXPECT_EQ(nullptr, token.getParent());
  EXPECT_EQ(1, token.getKind());
}

TEST(SyntaxTokenTest, ConstructorWithoutParent) {
  const auto green = createTestGreenToken();
  const auto token = SyntaxToken(10, 5, green);

  EXPECT_EQ(10, token.getOffset());
  EXPECT_EQ(5, token.getIndex());
  EXPECT_EQ(nullptr, token.getParent());
  EXPECT_EQ(1, token.getKind());
}

TEST(SyntaxTokenTest, CopyConstructorSharesData) {
  const auto green = createTestGreenToken();
  const auto token1 = SyntaxToken(25, 15, green);
  const auto token2 = SyntaxToken(token1);

  EXPECT_EQ(token1, token2);
  EXPECT_EQ(token1.getOffset(), token2.getOffset());
  EXPECT_EQ(token1.getIndex(), token2.getIndex());
  EXPECT_EQ(token1.getParent(), token2.getParent());
  EXPECT_EQ(token1.getKind(), token2.getKind());
}

TEST(SyntaxTokenTest, MultipleTokensCanShareData) {
  const auto green = createTestGreenToken();
  const auto token1 = SyntaxToken(200, 100, green);
  const auto token2 = SyntaxToken(token1);
  const auto token3 = SyntaxToken(token2);
  const auto token4 = SyntaxToken(token3);

  EXPECT_EQ(token1, token2);
  EXPECT_EQ(token2, token3);
  EXPECT_EQ(token3, token4);
  EXPECT_EQ(token1, token4);

  EXPECT_EQ(200, token4.getOffset());
  EXPECT_EQ(100, token4.getIndex());
  EXPECT_EQ(1, token4.getKind());
}

TEST(SyntaxNodeTest, EqualityOperatorWorksCorrectly) {
  const auto green1 = createTestGreenNode();
  const auto green2 = createTestGreenNode();

  const auto node1 = SyntaxNode(10, 5, nullptr, green1);
  const auto node2 = SyntaxNode(node1);
  const auto Node3 = SyntaxNode(10, 5, nullptr, green2);

  EXPECT_EQ(node1, node2);
  EXPECT_EQ(node1, Node3);
}

TEST(SyntaxTokenTest, EqualityOperatorWorksCorrectly) {
  const auto green1 = createTestGreenToken();
  const auto green2 = createTestGreenToken();

  const auto token1 = SyntaxToken(10, 5, green1);
  const auto token2 = SyntaxToken(token1);
  const auto token3 = SyntaxToken(10, 5, green2);

  EXPECT_EQ(token1, token2);
  EXPECT_EQ(token1, token3);
}

TEST(SyntaxDataTest, EqualityOperatorWorksCorrectly) {
  const auto green1 = createTestGreenNode();
  const auto green2 = createTestGreenNode();

  const auto data1 = SyntaxData(green1, nullptr, 10, 5);
  const auto data2 = SyntaxData(data1);
  const auto data3 = SyntaxData(green2, nullptr, 10, 5);
  const auto data4 = SyntaxData(green1, nullptr, 20, 5);

  EXPECT_EQ(data1, data2);
  EXPECT_EQ(data1, data3);
  EXPECT_NE(data1, data4);
}

TEST(SyntaxNodeTest, CopyAssignmentWorksCorrectly) {
  const auto green1 = createTestGreenNode();
  const auto green2 = createTestGreenNode();

  auto node1 = SyntaxNode(10, 5, nullptr, green1);
  auto node2 = SyntaxNode(20, 10, nullptr, green2);

  node2 = node1;

  EXPECT_EQ(node1, node2);
  EXPECT_EQ(10, node2.getOffset());
  EXPECT_EQ(5, node2.getIndex());
}

TEST(SyntaxTokenTest, CopyAssignmentWorksCorrectly) {
  const auto green1 = createTestGreenToken();
  const auto green2 = createTestGreenToken();

  auto token1 = SyntaxToken(10, 5, green1);
  auto token2 = SyntaxToken(20, 10, green2);

  token2 = token1;

  EXPECT_EQ(token1, token2);
  EXPECT_EQ(10, token2.getOffset());
  EXPECT_EQ(5, token2.getIndex());
}

} // namespace
