#ifndef YUZU_PARSER_EVENT_H
#define YUZU_PARSER_EVENT_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Parser/ParseError.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <variant>
#include <utility>

namespace yuzu::parser {
/// \brief Event marking the start of a syntax node.
///
/// StartEvent is emitted when the parser begins parsing a new syntax node.
/// It records the kind of node being parsed and optionally tracks a forward
/// parent relationship for handling precedence and associativity.
struct [[nodiscard]] StartEvent final {
  /// Optional index of a forward parent event for precedence handling.
  std::optional<size_t> forwardParent;

  /// The kind of syntax node being started.
  ast::SyntaxKind kind;
};

/// \brief Event marking the completion of a syntax node.
///
/// FinishEvent is emitted when the parser completes parsing a syntax node
/// that was previously started with a StartEvent.
struct [[nodiscard]] FinishEvent final {};

/// \brief Event representing a consumed token.
///
/// TokenEvent is emitted when the parser consumes a token from the lexer
/// and adds it to the current syntax node.
struct [[nodiscard]] TokenEvent final {
  ast::SyntaxKind kind;
};

/// \brief Event representing a parse error.
///
/// ErrorEvent is emitted when the parser encounters a syntax error and
/// needs to record it in the event stream.
struct [[nodiscard]] ErrorEvent final {
  std::unique_ptr<const ParseError> error;
};

/// \brief Event serving as a placeholder in the event stream.
///
/// PlaceholderEvent is used internally by the parser to reserve positions
/// in the event stream that will be replaced with actual events later.
struct [[nodiscard]] PlaceholderEvent final {};

/// \brief A variant type representing any parser event.
///
/// Event is a discriminated union that can hold any of the parser event
/// types: StartEvent, FinishEvent, TokenEvent, ErrorEvent, or PlaceholderEvent.
/// The parser uses these events to build the syntax tree incrementally.
class [[nodiscard]] Event final
    : public std::variant<StartEvent, FinishEvent, TokenEvent, ErrorEvent,
                          PlaceholderEvent> {
public:
  using std::variant<StartEvent, FinishEvent, TokenEvent, ErrorEvent,
                     PlaceholderEvent>::variant;

  Event() = delete;

  /// \brief Exchanges this event with a replacement and returns the old value.
  ///
  /// This method replaces the current event with the given replacement event
  /// and returns the original event. It uses move construction and placement
  /// new to avoid assignment, allowing Event to contain types with const
  /// members.
  ///
  /// \param replacement The event to store in place of the current event.
  /// \return The event that was previously stored.
  Event exchange(Event &&replacement) {
    Event old = std::move(*this);
    this->~Event();
    new (this) Event(std::move(replacement));
    return old;
  }
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_EVENT_H
