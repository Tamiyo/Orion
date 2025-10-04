#ifndef SYNTAX_GREEN_GREEN_H
#define SYNTAX_GREEN_GREEN_H

#include "Syntax/SyntaxKind.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace yuzu::syntax {
class GreenNode;
class GreenToken;

using GreenElement = std::variant<GreenNode, GreenToken>;

struct GreenTokenData {
  const std::u32string Source;
  const SyntaxKind Kind;
};

struct GreenNodeData {
  const GreenElement *const Children;
  const size_t NumChildren;
  const size_t Width;
  const SyntaxKind Kind;
};

class GreenToken {
public:
  explicit GreenToken(const SyntaxKind Kind, const std::u32string &Source)
      : Data_(std::make_shared<const GreenTokenData>(
            GreenTokenData{.Source = std::move(Source), .Kind = Kind})) {}

  GreenToken() = delete;

  [[nodiscard]] SyntaxKind getKind() const noexcept { return Data_->Kind; }

  [[nodiscard]] std::u32string_view getSource() const noexcept {
    return Data_->Source;
  }

  [[nodiscard]] size_t getWidth() const noexcept {
    return Data_->Source.size();
  }

  [[nodiscard]] size_t getUseCount() const noexcept {
    return Data_.use_count();
  }

  bool operator==(const GreenToken &Other) const noexcept {
    return Data_->Kind == Other.Data_->Kind &&
           Data_->Source == Other.Data_->Source;
  }

private:
  std::shared_ptr<const GreenTokenData> Data_;
};

class GreenNode {
public:
  class Iterator {
  public:
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = const GreenElement;
    using pointer = value_type *;
    using reference = value_type &;

    explicit Iterator(const GreenNode *Node, size_t Index)
        : Node_(Node), Index_(Index) {}

    Iterator() = delete;

    reference operator*() const { return Node_->Data_->Children[Index_]; }

    pointer operator->() const { return &(Node_->Data_->Children[Index_]); }

    Iterator &operator++() {
      ++Index_;
      return *this;
    }

    Iterator operator++(int) {
      Iterator tmp = *this;
      ++Index_;
      return tmp;
    }

    bool operator==(const Iterator &other) const {
      return Node_ == other.Node_ && Index_ == other.Index_;
    }

    bool operator!=(const Iterator &other) const { return !(*this == other); }

  private:
    const GreenNode *Node_;
    size_t Index_;
  };

  class Children {
  public:
    explicit Children(const GreenNode *Node) : Node_(Node) {}

    size_t size() const { return Node_->Data_->NumChildren; };

    Iterator begin() const { return Iterator(Node_, 0); }

    Iterator end() const { return Iterator(Node_, Node_->Data_->NumChildren); }

  private:
    const GreenNode *Node_;
  };

  explicit GreenNode(SyntaxKind Kind, GreenElement *Children,
                     size_t NumChildren, size_t Width);

  static GreenNode create(SyntaxKind Kind, std::vector<GreenElement> Children);

  GreenNode() = delete;

  [[nodiscard]] SyntaxKind getKind() const noexcept { return Data_->Kind; }

  [[nodiscard]] size_t getWidth() const noexcept { return Data_->Width; }

  [[nodiscard]] size_t getUseCount() const noexcept {
    return Data_.use_count();
  }

  [[nodiscard]] Children getChildren() const noexcept { return Children(this); }

  bool operator==(const GreenNode &Other) const noexcept;

  [[nodiscard]] static size_t
  computeWidth(const std::vector<GreenElement> &Children);

private:
  std::shared_ptr<const GreenNodeData> Data_;
};
} // namespace yuzu::syntax

#endif // SYNTAX_GREEN_GREEN_H
