#ifndef SYNTAX_PARSER_RGTREE_GREEN_GREEN_ELEMENT_H_
#define SYNTAX_PARSER_RGTREE_GREEN_GREEN_ELEMENT_H_

#include <optional>
#include <utility>
#include <variant>

#include "syntax/parser/rgtree/green/green_node.h"
#include "syntax/parser/rgtree/green/green_token.h"

namespace orion::syntax {

/**
 * @brief Represents a green element, which can be either a `GreenNode` or a
 * `GreenToken`.
 *
 * The `GreenElement` class uses a `std::variant` to hold either a node or a
 * token, allowing for flexible representation of syntax tree elements.
 */
class GreenElement {
 public:
  /**
   * @brief Constructs a `GreenElement` from a `GreenNode`.
   *
   * @param node The `GreenNode` to be stored in the element.
   */
  explicit GreenElement(GreenNode node) : variant_(std::move(node)) {}

  /**
   * @brief Constructs a `GreenElement` from a `GreenToken`.
   *
   * @param token The `GreenToken` to be stored in the element.
   */
  explicit GreenElement(GreenToken token) : variant_(std::move(token)) {}

  /**
   * @brief Default constructor that initializes the element to an empty state.
   */
  explicit GreenElement() : variant_(std::monostate()) {}

  /**
   * @brief Constructs a `GreenElement` from a const reference to a `GreenNode`.
   *
   * @param node The `GreenNode` to be stored in the element.
   */
  GreenElement(const GreenNode& node) : variant_(std::move(node)) {}

  /**
   * @brief Constructs a `GreenElement` from a const reference to a
   * `GreenToken`.
   *
   * @param token The `GreenToken` to be stored in the element.
   */
  GreenElement(const GreenToken& token) : variant_(std::move(token)) {}

  /** Defaulted copy and move constructors. */
  GreenElement(const GreenElement&) = default;
  GreenElement(GreenElement&&) = default;

  /**
   * @brief Move assignment operator.
   *
   * @param other The `GreenElement` to move from.
   * @return A reference to this `GreenElement`.
   */
  GreenElement& operator=(GreenElement&& other) noexcept { return other; }

  /**
   * @brief Checks if the element holds a `GreenNode`.
   *
   * @return `true` if the element is a `GreenNode`, otherwise `false`.
   */
  [[nodiscard]] bool IsNode() const noexcept {
    return std::holds_alternative<GreenNode>(variant_);
  }

  /**
   * @brief Checks if the element holds a `GreenToken`.
   *
   * @return `true` if the element is a `GreenToken`, otherwise `false`.
   */
  [[nodiscard]] bool IsToken() const noexcept {
    return std::holds_alternative<GreenToken>(variant_);
  }

  /**
   * @brief Attempts to retrieve the stored `GreenNode`.
   *
   * @return An optional containing the `GreenNode` if it is present, otherwise
   * `nullopt`.
   */
  [[nodiscard]] std::optional<GreenNode> TryGetNode() const noexcept {
    if (std::holds_alternative<GreenNode>(variant_)) {
      return std::make_optional(std::get<GreenNode>(variant_));
    }

    return std::nullopt;
  }

  /**
   * @brief Attempts to retrieve the stored `GreenToken`.
   *
   * @return An optional containing the `GreenToken` if it is present, otherwise
   * `nullopt`.
   */
  [[nodiscard]] std::optional<GreenToken> TryGetToken() const noexcept {
    if (std::holds_alternative<GreenToken>(variant_)) {
      return std::make_optional(std::get<GreenToken>(variant_));
    }

    return std::nullopt;
  }

  /**
   * @brief Returns the current use count of the stored element's data.
   *
   * @return The number of instances sharing the same `GreenNode` or
   * `GreenToken`.
   */
  [[nodiscard]] size_t UseCount() const noexcept {
    if (std::holds_alternative<GreenNode>(variant_)) {
      return std::get<GreenNode>(variant_).UseCount();
    }

    if (std::holds_alternative<GreenToken>(variant_)) {
      return std::get<GreenToken>(variant_).UseCount();
    }

    return 0;  // No shared data for monostate.
  }

  /**
   * @brief Compares two `GreenElement` objects for equality.
   *
   * @param other The other `GreenElement` to compare with.
   * @return `true` if both elements are equal, otherwise `false`.
   */
  bool operator==(const GreenElement& other) const noexcept {
    return variant_ == other.variant_;
  }

 private:
  /** Variant that can hold either a `GreenNode`, `GreenToken`, or a
   * `monostate`. */
  const std::variant<GreenNode, GreenToken, std::monostate> variant_;
};

}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_RGTREE_GREEN_GREEN_ELEMENT_H_
