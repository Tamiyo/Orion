#include "yuzu/Syntax/Syntax.h"

#include <gtest/gtest.h>

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
  const auto Green = createTestGreenNode();
  const auto Data = SyntaxData(Green, nullptr, 0, 0);

  EXPECT_EQ(1, Data.getRc()->load());
}

TEST(SyntaxDataTest, CopyConstructorSharesRefCount) {
  const auto Green = createTestGreenNode();
  const auto Data1 = SyntaxData(Green, nullptr, 0, 0);

  const auto Data2 = SyntaxData(Data1);

  EXPECT_EQ(2, Data1.getRc()->load());
  EXPECT_EQ(2, Data2.getRc()->load());
  EXPECT_EQ(Data1.getRc(), Data2.getRc());
}

TEST(SyntaxDataTest, CopyAssignmentHandlesRefCount) {
  const auto Green1 = createTestGreenNode();
  const auto Green2 = createTestGreenToken();

  auto Data1 = SyntaxData(Green1, nullptr, 0, 0);
  auto Data2 = SyntaxData(Green2, nullptr, 10, 1);

  Data2 = Data1;

  EXPECT_EQ(Data1.getOffset(), Data2.getOffset());
  EXPECT_EQ(Data1.getIndex(), Data2.getIndex());

  EXPECT_EQ(2, Data1.getRc()->load());
  EXPECT_EQ(2, Data2.getRc()->load());
  EXPECT_EQ(Data1.getRc(), Data2.getRc());
}

TEST(SyntaxDataTest, SelfAssignmentIsNoop) {
  const auto Green = createTestGreenNode();
  auto Data = SyntaxData(Green, nullptr, 42, 7);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-assign-overloaded"
  Data = Data;
#pragma GCC diagnostic pop

  EXPECT_EQ(42, Data.getOffset());
  EXPECT_EQ(7, Data.getIndex());

  EXPECT_EQ(1, Data.getRc()->load());
}

TEST(SyntaxDataTest, MoveConstructorTransfersOwnership) {
  const auto Green = createTestGreenNode();
  auto Data1 = SyntaxData(Green, nullptr, 0, 0);

  const auto Data2 = SyntaxData(std::move(Data1));

  EXPECT_EQ(0, Data2.getOffset());
  EXPECT_EQ(0, Data2.getIndex());

  EXPECT_EQ(1, Data2.getRc()->load());
}

TEST(SyntaxDataTest, MoveAssignmentTransfersOwnership) {
  const auto Green1 = createTestGreenNode();
  const auto Green2 = createTestGreenToken();

  auto Data1 = SyntaxData(Green1, nullptr, 10, 5);
  auto Data2 = SyntaxData(Green2, nullptr, 20, 10);

  Data2 = std::move(Data1);

  EXPECT_EQ(10, Data2.getOffset());
  EXPECT_EQ(5, Data2.getIndex());

  EXPECT_EQ(1, Data2.getRc()->load());
}

TEST(SyntaxDataTest, GettersReturnCorrectValues) {
  const auto Green = createTestGreenNode();
  SyntaxData *const Parent = nullptr;
  const size_t Offset = 42;
  const size_t Index = 7;

  const auto Data = SyntaxData(Green, Parent, Offset, Index);

  EXPECT_EQ(Parent, Data.getParent());
  EXPECT_EQ(Offset, Data.getOffset());
  EXPECT_EQ(Index, Data.getIndex());
}

TEST(SyntaxDataTest, MultipleReferencesShareData) {
  const auto Green = createTestGreenNode();
  const auto Data1 = SyntaxData(Green, nullptr, 0, 0);

  const auto Data2 = SyntaxData(Data1);
  const auto Data3 = SyntaxData(Data2);
  const auto Data4 = Data3;

  EXPECT_EQ(Data1.getOffset(), Data4.getOffset());

  EXPECT_EQ(4, Data1.getRc()->load());
  EXPECT_EQ(4, Data2.getRc()->load());
  EXPECT_EQ(4, Data3.getRc()->load());
  EXPECT_EQ(4, Data4.getRc()->load());

  EXPECT_EQ(Data1.getRc(), Data2.getRc());
  EXPECT_EQ(Data2.getRc(), Data3.getRc());
  EXPECT_EQ(Data3.getRc(), Data4.getRc());
}

TEST(SyntaxNodeTest, ConstructorCreatesNode) {
  const auto Green = createTestGreenNode();
  const auto Node = SyntaxNode(10, 5, nullptr, Green);

  EXPECT_EQ(10, Node.getOffset());
  EXPECT_EQ(5, Node.getIndex());
  EXPECT_EQ(nullptr, Node.getParent());
  EXPECT_EQ(2, Node.getKind());
}

TEST(SyntaxNodeTest, CreateRootCreatesRootNode) {
  const auto Green = createTestGreenNode();
  const auto Root = SyntaxNode::createRoot(Green);

  EXPECT_EQ(0, Root.getOffset());
  EXPECT_EQ(0, Root.getIndex());
  EXPECT_EQ(nullptr, Root.getParent());
  EXPECT_EQ(2, Root.getKind());
}

TEST(SyntaxNodeTest, CopyConstructorSharesData) {
  const auto Green = createTestGreenNode();
  const auto Node1 = SyntaxNode(42, 7, nullptr, Green);

  const auto Node2 = SyntaxNode(Node1);

  EXPECT_EQ(Node1, Node2);
  EXPECT_EQ(Node1.getOffset(), Node2.getOffset());
  EXPECT_EQ(Node1.getIndex(), Node2.getIndex());
  EXPECT_EQ(Node1.getParent(), Node2.getParent());
  EXPECT_EQ(Node1.getKind(), Node2.getKind());
}

TEST(SyntaxNodeTest, MultipleNodesCanShareData) {
  const auto Green = createTestGreenNode();
  const auto Node1 = SyntaxNode(100, 50, nullptr, Green);

  const auto Node2 = SyntaxNode(Node1);
  const auto Node3 = SyntaxNode(Node2);
  const auto Node4 = SyntaxNode(Node3);

  EXPECT_EQ(Node1, Node2);
  EXPECT_EQ(Node2, Node3);
  EXPECT_EQ(Node3, Node4);
  EXPECT_EQ(Node1, Node4);

  EXPECT_EQ(100, Node4.getOffset());
  EXPECT_EQ(50, Node4.getIndex());
}

TEST(SyntaxTokenTest, ConstructorWithParent) {
  const auto Green = createTestGreenToken();
  const auto Token = SyntaxToken(10, 5, nullptr, Green);

  EXPECT_EQ(10, Token.getOffset());
  EXPECT_EQ(5, Token.getIndex());
  EXPECT_EQ(nullptr, Token.getParent());
  EXPECT_EQ(1, Token.getKind());
}

TEST(SyntaxTokenTest, ConstructorWithoutParent) {
  const auto Green = createTestGreenToken();
  const auto Token = SyntaxToken(10, 5, Green);

  EXPECT_EQ(10, Token.getOffset());
  EXPECT_EQ(5, Token.getIndex());
  EXPECT_EQ(nullptr, Token.getParent());
  EXPECT_EQ(1, Token.getKind());
}

TEST(SyntaxTokenTest, CopyConstructorSharesData) {
  const auto Green = createTestGreenToken();
  const auto Token1 = SyntaxToken(25, 15, Green);
  const auto Token2 = SyntaxToken(Token1);

  EXPECT_EQ(Token1, Token2);
  EXPECT_EQ(Token1.getOffset(), Token2.getOffset());
  EXPECT_EQ(Token1.getIndex(), Token2.getIndex());
  EXPECT_EQ(Token1.getParent(), Token2.getParent());
  EXPECT_EQ(Token1.getKind(), Token2.getKind());
}

TEST(SyntaxTokenTest, MultipleTokensCanShareData) {
  const auto Green = createTestGreenToken();
  const auto Token1 = SyntaxToken(200, 100, Green);
  const auto Token2 = SyntaxToken(Token1);
  const auto Token3 = SyntaxToken(Token2);
  const auto Token4 = SyntaxToken(Token3);

  EXPECT_EQ(Token1, Token2);
  EXPECT_EQ(Token2, Token3);
  EXPECT_EQ(Token3, Token4);
  EXPECT_EQ(Token1, Token4);

  EXPECT_EQ(200, Token4.getOffset());
  EXPECT_EQ(100, Token4.getIndex());
  EXPECT_EQ(1, Token4.getKind());
}

TEST(SyntaxNodeTest, EqualityOperatorWorksCorrectly) {
  const auto Green1 = createTestGreenNode();
  const auto Green2 = createTestGreenNode();

  const auto Node1 = SyntaxNode(10, 5, nullptr, Green1);
  const auto Node2 = SyntaxNode(Node1);
  const auto Node3 = SyntaxNode(10, 5, nullptr, Green2);

  EXPECT_EQ(Node1, Node2);
  EXPECT_NE(Node1, Node3);
}

TEST(SyntaxTokenTest, EqualityOperatorWorksCorrectly) {
  const auto Green1 = createTestGreenToken();
  const auto Green2 = createTestGreenToken();

  const auto Token1 = SyntaxToken(10, 5, Green1);
  const auto Token2 = SyntaxToken(Token1);
  const auto Token3 = SyntaxToken(10, 5, Green2);

  EXPECT_EQ(Token1, Token2);
  EXPECT_NE(Token1, Token3);
}

TEST(SyntaxDataTest, EqualityOperatorWorksCorrectly) {
  const auto Green1 = createTestGreenNode();
  const auto Green2 = createTestGreenNode();

  const auto Data1 = SyntaxData(Green1, nullptr, 10, 5);
  const auto Data2 = SyntaxData(Data1);
  const auto Data3 = SyntaxData(Green2, nullptr, 10, 5);
  const auto Data4 = SyntaxData(Green1, nullptr, 20, 5);

  EXPECT_EQ(Data1, Data2);
  EXPECT_NE(Data1, Data3);
  EXPECT_NE(Data1, Data4);
}

TEST(SyntaxNodeTest, CopyAssignmentWorksCorrectly) {
  const auto Green1 = createTestGreenNode();
  const auto Green2 = createTestGreenNode();

  auto Node1 = SyntaxNode(10, 5, nullptr, Green1);
  auto Node2 = SyntaxNode(20, 10, nullptr, Green2);

  Node2 = Node1;

  EXPECT_EQ(Node1, Node2);
  EXPECT_EQ(10, Node2.getOffset());
  EXPECT_EQ(5, Node2.getIndex());
}

TEST(SyntaxTokenTest, CopyAssignmentWorksCorrectly) {
  const auto Green1 = createTestGreenToken();
  const auto Green2 = createTestGreenToken();

  auto Token1 = SyntaxToken(10, 5, Green1);
  auto Token2 = SyntaxToken(20, 10, Green2);

  Token2 = Token1;

  EXPECT_EQ(Token1, Token2);
  EXPECT_EQ(10, Token2.getOffset());
  EXPECT_EQ(5, Token2.getIndex());
}

} // namespace
