#ifndef YUZU_PARSING_SPAN_H
#define YUZU_PARSING_SPAN_H

#include <cstdint>

namespace yuzu::parsing {
struct Span final {
  const uint32_t start;
  const uint32_t end;

  bool operator==(const Span &other) const {
    return start == other.start && end == other.end;
  }
};
} // namespace yuzu::parsing

#endif // YUZU_PARSING_SPAN_H
