#ifndef YUZU_PARSER_EVENT_H
#define YUZU_PARSER_EVENT_H

#include "yuzu/Ast/SyntaxKind.h"

#include <cstddef>
#include <optional>
#include <variant>

namespace yuzu::parser {
/// \brief Event marking the start of a syntax node.
///
/// StartEvent is emitted when the parser begins parsing a new syntax node.
/// It records the kind of node being parsed and optionally tracks a forward
/// parent relationship for handling precedence and associativity.
struct StartEvent final {
  /// Optional index of a forward parent event for precedence handling.
  std::optional<size_t> forwardParent;

  /// The kind of syntax node being started.
  const ast::SyntaxKind kind;
};

/// \brief Event marking the completion of a syntax node.
///
/// FinishEvent is emitted when the parser completes parsing a syntax node
/// that was previously started with a StartEvent.
struct FinishEvent final {};

/// \brief Event representing a consumed token.
///
/// TokenEvent is emitted when the parser consumes a token from the lexer
/// and adds it to the current syntax node.
struct TokenEvent final {};

/// \brief Event representing a parse error.
///
/// ErrorEvent is emitted when the parser encounters a syntax error and
/// needs to record it in the event stream.
struct ErrorEvent final {};

/// \brief Event serving as a placeholder in the event stream.
///
/// PlaceholderEvent is used internally by the parser to reserve positions
/// in the event stream that will be replaced with actual events later.
struct PlaceholderEvent final {};

/// \brief A variant type representing any parser event.
///
/// Event is a discriminated union that can hold any of the parser event
/// types: StartEvent, FinishEvent, TokenEvent, ErrorEvent, or PlaceholderEvent.
/// The parser uses these events to build the syntax tree incrementally.
class Event final : public std::variant<StartEvent, FinishEvent, TokenEvent,
                                        ErrorEvent, PlaceholderEvent> {
  using std::variant<StartEvent, FinishEvent, TokenEvent, ErrorEvent,
                     PlaceholderEvent>::variant;

  /// Deleted default constructor to enforce proper initialization.
  Event() = delete;
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_EVENT_H
