#include "syntax/parser/token_sink.h"

#include <ranges>
#include <stdexcept>
#include <variant>
#include <vector>

#include "syntax/parser/event/event.h"
#include "syntax/syntax_kind.h"

namespace orion::syntax {

void TokenSink::Finish() {
  for (size_t event_idx = 0; event_idx < events_.size(); event_idx++) {
    if (const Event event = events_[event_idx];
        std::holds_alternative<StartEvent>(event)) {
      const auto [kind, forward_parent] = std::get<StartEvent>(event);
      events_[event_idx] = PlaceholderEvent{};

      StartNode(event_idx, kind, forward_parent);
    } else if (std::holds_alternative<FinishEvent>(event)) {
      FinishNode();
    } else if (std::holds_alternative<TokenEvent>(event)) {
      AddToken();
    } else if (std::holds_alternative<PlaceholderEvent>(event)) {
      // Do Nothing
    } else {
      throw std::invalid_argument("unknown event type");
    }
  }
}

void TokenSink::StartNode(const size_t event_idx, const SyntaxKind kind,
                          const std::optional<size_t> forward_parent) {
  size_t event_idx_mut = event_idx;
  std::optional<size_t> forward_parent_mut = forward_parent;

  std::vector kinds = {kind};
  while (forward_parent_mut.has_value()) {
    event_idx_mut += forward_parent_mut.value();

    if (const Event event = events_[event_idx_mut];
        std::holds_alternative<StartEvent>(event)) {
      const auto [kind, forward_parent] = std::get<StartEvent>(event);
      events_[event_idx_mut] = PlaceholderEvent{};

      kinds.emplace_back(kind);
      forward_parent_mut = forward_parent;
    } else {
      throw std::invalid_argument("unreachable event in StartNode");
    }
  }

  for (auto & k : std::ranges::reverse_view(kinds)) {
    builder_.StartNode(k);
  }
}
}  // namespace orion::syntax
