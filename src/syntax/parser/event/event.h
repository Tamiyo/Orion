#ifndef SYNTAX_PARSER_EVENT_EVENT_H_
#define SYNTAX_PARSER_EVENT_EVENT_H_

#include <optional>
#include <variant>

#include "syntax/syntax_kind.h"

namespace orion::syntax {
struct StartEvent {
  const SyntaxKind kind;
  const std::optional<size_t> forward_parent;
};

struct FinishEvent {};

struct TokenEvent {};

struct ErrorEvent {};

struct PlaceholderEvent {};

using Event = std::variant<StartEvent, FinishEvent, TokenEvent, ErrorEvent,
                           PlaceholderEvent>;
}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_EVENT_EVENT_H_
