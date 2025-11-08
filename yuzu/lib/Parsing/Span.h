#ifndef YUZU_PARSING_SPAN_H
#define YUZU_PARSING_SPAN_H

#include <cstdint>

namespace yuzu::parsing {
struct Span final {
  const uint32_t Start;
  const uint32_t End;

  bool operator==(const Span &Other) const {
    return Start == Other.Start && End == Other.End;
  }
};
} // namespace yuzu::parsing

#endif // YUZU_PARSING_SPAN_H
