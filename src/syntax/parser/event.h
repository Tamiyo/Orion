#ifndef SYNTAX_PARSER_EVENT_H_
#define SYNTAX_PARSER_EVENT_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

#include "syntax/parser/error/parse_error.h"

namespace yuzu::syntax {
/// \brief Marks the beginning of a syntax node.
/// Contains the kind of the node and an optional forward parent index,
/// which allows referencing a parent node that hasn't yet been completed.
template <typename SyntaxKind = uint16_t>
class StartEvent {
 public:
  explicit StartEvent(SyntaxKind kind,
                      std::optional<size_t> forward_parent = std::nullopt)
      : kind_(kind), forward_parent_(forward_parent) {}

  /// \return The kind of the syntax node being started.
  [[nodiscard]] SyntaxKind Kind() const noexcept { return this->kind_; }

  /// \return An optional index to a forward parent node.
  [[nodiscard]] std::optional<size_t> ForwardParent() const noexcept {
    return this->forward_parent_;
  }

  /// \brief Sets the index of the forward parent.
  void SetForwardParent(const size_t forward_parent) noexcept {
    forward_parent_ = forward_parent;
  }

 private:
  SyntaxKind kind_;
  std::optional<size_t> forward_parent_;
};

/// \brief Marks the end of a syntax node.
class FinishEvent {};

/// \brief Represents a single token in the syntax stream.
/// Typically used when a token is consumed during parsing.
class TokenEvent {};

/// \brief Represents a parse error.
/// Stores the error that occurred during parsing.
template <typename TokenKind = uint16_t>
class ErrorEvent {
 public:
  explicit ErrorEvent(const ParseError<TokenKind>& error)
      : error_(std::move(error)) {}

  /// \return The parse error associated with this event.
  [[nodiscard]] const ParseError<TokenKind>& Error() const noexcept {
    return error_;
  }

 private:
  const ParseError<TokenKind> error_;
};

/// \brief Placeholder event used as a sentinel or to overwrite used events.
/// This can be useful when modifying or reusing the event stream.
class PlaceholderEvent {};

/// \brief Represents any event that can occur during parsing.
/// This includes node boundaries, tokens, errors, and placeholders.
template <typename TokenKind = uint16_t, typename SyntaxKind = uint16_t>
using Event = std::variant<StartEvent<SyntaxKind>, FinishEvent, TokenEvent,
                           ErrorEvent<TokenKind>, PlaceholderEvent>;

}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_EVENT_H_
