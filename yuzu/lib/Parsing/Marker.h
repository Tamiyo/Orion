#ifndef YUZU_PARSING_MARKER_H
#define YUZU_PARSING_MARKER_H

#include <cstddef>

namespace yuzu::parsing {
struct CompletedMarker final {
  const size_t position;
};

struct Marker final {
  const size_t position;
};
} // namespace yuzu::parsing

#endif // YUZU_PARSING_MARKER_H
