#ifndef YUZU_PARSING_EVENT_H
#define YUZU_PARSING_EVENT_H

#include "yuzu/Ast/Syntax.h"

#include <cstddef>
#include <optional>
#include <variant>

namespace yuzu::parsing {
struct StartEvent final {
  std::optional<size_t> forwardParent;
  const ast::SyntaxKind kind;
};

struct FinishEvent final {};

struct TokenEvent final {};

struct ErrorEvent final {};

struct PlaceholderEvent final {};

class Event final : public std::variant<StartEvent, FinishEvent, TokenEvent,
                                        ErrorEvent, PlaceholderEvent> {
  using std::variant<StartEvent, FinishEvent, TokenEvent, ErrorEvent,
                     PlaceholderEvent>::variant;

  Event() = delete;
};
} // namespace yuzu::parsing

#endif // YUZU_PARSING_EVENT_H
