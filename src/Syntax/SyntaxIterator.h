#ifndef SYNTAX_SYNTAX_ITERATOR_H
#define SYNTAX_SYNTAX_ITERATOR_H

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"
#include "Util/ErrorHandling.h"

namespace yuzu::syntax {
class SyntaxIteratorWithoutTokens {
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = const SyntaxNode;
  using pointer = value_type *;
  using reference = value_type &;

  explicit SyntaxIteratorWithoutTokens(GreenNode::Iterator It,
                                       const SyntaxNode &Parent)
      : It_(It), Parent_(&Parent), Offset_(Parent.getOffset()) {
    skipTokens();
  }

  SyntaxIteratorWithoutTokens() = delete;

  value_type operator*() const {
    if (const GreenNode *Node = std::get_if<GreenNode>(&(*It_))) {
      return SyntaxNode(Offset_, Parent_, *Node);
    }

    util::yuzu_unreachable();
  }

  SyntaxIteratorWithoutTokens &operator++() {
    if (const GreenNode *Node = std::get_if<GreenNode>(&(*It_))) {
      Offset_ += Node->getWidth();

      ++It_;
      skipTokens();

      return *this;
    }

    util::yuzu_unreachable();
  }

  SyntaxIteratorWithoutTokens operator++(int) {
    SyntaxIteratorWithoutTokens Tmp = *this;
    ++(*this);
    return Tmp;
  }

  friend bool operator==(const SyntaxIteratorWithoutTokens &A,
                         const SyntaxIteratorWithoutTokens &B) {
    return A.It_ == B.It_ && A.Parent_ == B.Parent_;
  }

  friend bool operator!=(const SyntaxIteratorWithoutTokens &A,
                         const SyntaxIteratorWithoutTokens &B) {
    return !(A == B);
  }

private:
  inline void skipTokens() {
    const GreenNode::Iterator End = Parent_->getGreen().getChildren().end();

    while (It_ != End) {
      if (const GreenNode *_ = std::get_if<GreenNode>(&(*It_))) {
        return;
      }

      if (const GreenToken *Token = std::get_if<GreenToken>(&(*It_))) {
        Offset_ += Token->getWidth();
        ++It_;
        continue;
      }

      util::yuzu_unreachable();
    }
  }

  GreenNode::Iterator It_;
  const SyntaxNode *const Parent_;
  size_t Offset_;
};

class SyntaxIteratorWithTokens {
public:
  using iterator_category = std::forward_iterator_tag;
  using difference_type = std::ptrdiff_t;
  using value_type = SyntaxElement;
  using pointer = value_type *;
  using reference = value_type &;

  explicit SyntaxIteratorWithTokens(GreenNode::Iterator It,
                                    const SyntaxNode &Parent)
      : It_(It), Parent_(&Parent), Offset_(Parent.getOffset()) {}

  SyntaxIteratorWithTokens() = delete;

  value_type operator*() const {
    if (const GreenNode *Node = std::get_if<GreenNode>(&(*It_))) {
      return SyntaxNode(Offset_, Parent_, *Node);
    }

    if (const GreenToken *Token = std::get_if<GreenToken>(&(*It_))) {
      return SyntaxToken(Offset_, Parent_, *Token);
    }

    util::yuzu_unreachable();
  }

  SyntaxIteratorWithTokens &operator++() {
    if (const GreenNode *Node = std::get_if<GreenNode>(&(*It_))) {
      Offset_ += Node->getWidth();
      ++It_;
      return *this;
    }

    if (const GreenToken *Token = std::get_if<GreenToken>(&(*It_))) {
      Offset_ += Token->getWidth();
      ++It_;
      return *this;
    }

    util::yuzu_unreachable();
  }

  SyntaxIteratorWithTokens operator++(int) {
    SyntaxIteratorWithTokens Tmp = *this;
    ++(*this);
    return Tmp;
  }

  friend bool operator==(const SyntaxIteratorWithTokens &A,
                         const SyntaxIteratorWithTokens &B) {
    return A.It_ == B.It_ && A.Parent_ == B.Parent_;
  }

  friend bool operator!=(const SyntaxIteratorWithTokens &A,
                         const SyntaxIteratorWithTokens &B) {
    return !(A == B);
  }

private:
  GreenNode::Iterator It_;
  const SyntaxNode *const Parent_;
  size_t Offset_;
};

class SyntaxChildrenWithoutTokens {
public:
  using Iterator = SyntaxIteratorWithoutTokens;

  explicit SyntaxChildrenWithoutTokens(const SyntaxNode *Node) : Node_(Node) {}
  SyntaxChildrenWithoutTokens() = delete;

  Iterator begin() const noexcept {
    return Iterator(Node_->getGreen().getChildren().begin(), *Node_);
  }

  Iterator end() const noexcept {
    return Iterator(Node_->getGreen().getChildren().end(), *Node_);
  }

private:
  const SyntaxNode *const Node_;
};

class SyntaxChildrenWithTokens {
public:
  using Iterator = SyntaxIteratorWithTokens;

  explicit SyntaxChildrenWithTokens(const SyntaxNode *Node) : Node_(Node) {}
  SyntaxChildrenWithTokens() = delete;

  Iterator begin() const noexcept {
    return Iterator(Node_->getGreen().getChildren().begin(), *Node_);
  }

  Iterator end() const noexcept {
    return Iterator(Node_->getGreen().getChildren().end(), *Node_);
  }

private:
  const SyntaxNode *const Node_;
};
} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_ITERATOR_H
