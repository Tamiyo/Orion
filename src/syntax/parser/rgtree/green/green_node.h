#ifndef SYNTAX_PARSER_RGTREE_GREEN_GREEN_NODE_H_
#define SYNTAX_PARSER_RGTREE_GREEN_GREEN_NODE_H_

#include <functional>
#include <memory>
#include <utility>
#include <vector>

#include "syntax/parser/syntax_kind.h"

namespace orion::syntax {

class GreenElement;

/**
 * @brief Represents the data associated with a green node.
 *
 * `GreenNodeData` holds information about the kind of node, its width,
 * and its child elements. This structure is used during parsing and
 * tree construction.
 */
class GreenNodeData {
 public:
  /**
   * @brief Constructs a `GreenNodeData` with the specified token kind, width,
   * and child elements.
   *
   * @param kind The type of the node as defined by `SyntaxKind`.
   * @param width The width of the node in terms of layout.
   * @param children The child elements contained within this node.
   */
  explicit GreenNodeData(SyntaxKind kind, size_t width,
                         std::vector<GreenElement> children)
      : kind_(kind), width_(width), children_(std::move(children)) {}

  /**
   * @brief Deleted default constructor.
   *
   * A `GreenNodeData` must always be constructed with a kind, width, and
   * children.
   */
  GreenNodeData() = delete;

  /** Defaulted copy and move constructors. */
  GreenNodeData(const GreenNodeData&) = default;
  GreenNodeData(GreenNodeData&&) = default;

  /**
   * @brief Returns the kind of the node.
   *
   * @return The node's `SyntaxKind`.
   */
  [[nodiscard]] SyntaxKind Kind() const { return kind_; }

  /**
   * @brief Returns the width of the node.
   *
   * @return The width of the node.
   */
  [[nodiscard]] size_t Width() const { return width_; }

  /**
   * @brief Returns the child elements of the node.
   *
   * @return A reference to the vector of child `GreenElement`s.
   */
  [[nodiscard]] const std::vector<GreenElement>& Children() const {
    return children_;
  }

  /**
   * @brief Compares two `GreenNodeData` objects for equality.
   *
   * @param other The other `GreenNodeData` to compare with.
   * @return `true` if both nodes have the same kind, width, and children,
   * otherwise `false`.
   */
  bool operator==(const GreenNodeData& other) const {
    return kind_ == other.kind_ && width_ == other.width_ &&
           children_ == other.children_;
  }

 private:
  /**< The type of the node. */
  const SyntaxKind kind_;

  /**< The width of the node. */
  const size_t width_;

  /**< The child elements of the node. */
  const std::vector<GreenElement> children_;
};

/**
 * @brief Represents a green node in the syntax tree.
 *
 * `GreenNode` encapsulates `GreenNodeData` and provides access to node
 * properties and methods for interacting with the node's children.
 */
class GreenNode {
 public:
  /**
   * @brief Constructs a `GreenNode` with the specified kind and child elements.
   *
   * @param kind The type of the node as defined by `SyntaxKind`.
   * @param children The child elements contained within this node.
   */
  explicit GreenNode(SyntaxKind kind, const std::vector<GreenElement>& children)
      : data_(std::make_shared<GreenNodeData>(
            GreenNodeData(kind, ComputeWidth(children), children))) {}

  /**
   * @brief Deleted default constructor.
   *
   * A `GreenNode` must always be constructed with a kind and children.
   */
  GreenNode() = delete;

  /** Defaulted copy and move constructors. */
  GreenNode(const GreenNode&) = default;
  GreenNode(GreenNode&&) noexcept = default;

  /**
   * @brief Returns the kind of the node.
   *
   * @return The node's `SyntaxKind`.
   */
  [[nodiscard]] SyntaxKind Kind() const { return data_->Kind(); }

  /**
   * @brief Returns the width of the node.
   *
   * @return The width of the node.
   */
  [[nodiscard]] size_t Width() const { return data_->Width(); }

  /**
   * @brief Returns the child elements of the node.
   *
   * @return A reference to the vector of child `GreenElement`s.
   */
  [[nodiscard]] const std::vector<GreenElement>& Children() const {
    return data_->Children();
  }

  /**
   * @brief Returns the current use count of the shared node data.
   *
   * @return The number of `GreenNode` instances sharing the same
   * `GreenNodeData`.
   */
  [[nodiscard]] size_t UseCount() const { return data_.use_count(); }

  /**
   * @brief Compares two `GreenNode` objects for equality.
   *
   * @param other The other `GreenNode` to compare with.
   * @return `true` if both nodes share the same underlying data, otherwise
   * `false`.
   */
  bool operator==(const GreenNode& other) const { return data_ == other.data_; }

 private:
  /**
   * @brief Computes the width of the node based on its children.
   *
   * @param children The child elements to compute the width for.
   * @return The computed width of the node.
   */
  [[nodiscard]] static size_t ComputeWidth(
      const std::vector<GreenElement>& children);

  /**< Shared data for the node. */
  const std::shared_ptr<GreenNodeData> data_;
};

}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_RGTREE_GREEN_GREEN_NODE_H_
