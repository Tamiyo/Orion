#include "syntax/parser/token_sink.h"

#include <ranges>
#include <stdexcept>
#include <variant>
#include <vector>

#include "syntax/parser/event.h"
#include "syntax/syntax_kind.h"

namespace orion::syntax {
TokenSink::Result TokenSink::Finish() {
  for (size_t event_idx = 0; event_idx < events_.size(); event_idx++) {
    if (const Event event = events_[event_idx];
        event.HoldsAlternative<Event::Start>()) {
      const auto e = event.Get<Event::Start>();
      events_[event_idx] = Event::CreatePlaceholder();

      StartNode(event_idx, e.Kind(), e.ForwardParent());
    } else if (event.HoldsAlternative<Event::Finish>()) {
      FinishNode();
    } else if (event.HoldsAlternative<Event::Token>()) {
      AddToken();
    } else if (event.HoldsAlternative<Event::Placeholder>()) {
      // Do Nothing
    } else {
      throw std::invalid_argument("unknown event type");
    }
  }

  return TokenSink::Result{builder_.Finish()};
}

void TokenSink::StartNode(const size_t event_idx, const SyntaxKind kind,
                          const std::optional<size_t> forward_parent) {
  size_t event_idx_mut = event_idx;
  std::optional<size_t> forward_parent_mut = forward_parent;

  std::vector kinds = {kind};
  while (forward_parent_mut.has_value()) {
    event_idx_mut += forward_parent_mut.value();

    if (const Event event = events_[event_idx_mut];
        event.HoldsAlternative<Event::Start>()) {
      const auto e = event.Get<Event::Start>();
      events_[event_idx_mut] = Event::CreatePlaceholder();

      kinds.emplace_back(e.Kind());
      forward_parent_mut = e.ForwardParent();
    } else {
      throw std::invalid_argument("unreachable event in StartNode");
    }
  }

  for (auto& k : std::ranges::reverse_view(kinds)) {
    builder_.StartNode(k);
  }
}
}  // namespace orion::syntax
