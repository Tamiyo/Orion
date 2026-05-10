#ifndef YUZU_DIAGNOSTICS_SPAN_H
#define YUZU_DIAGNOSTICS_SPAN_H

#include <cstdint>

namespace yuzu::diagnostics {

/// \brief Opaque handle for a source registered with a `SourceMap`.
///
/// "Source" is broader than "file": a `SourceId` may refer to a real file
/// on disk, an in-memory REPL line, a synthetic test fixture, or anything
/// else fed through the diagnostics layer. The `SourceMap` decides what
/// the human-readable name of each id is (e.g. `"src/main.yuzu"`,
/// `"<repl>"`, `"<test>"`).
///
/// Trivially-copyable so spans stay small. `SourceId{}` is the invalid
/// sentinel — used as a placeholder only; real spans always carry a
/// registered id.
struct [[nodiscard]] SourceId final {
  uint32_t value = 0;

  bool operator==(const SourceId &other) const { return value == other.value; }
  bool operator!=(const SourceId &other) const { return value != other.value; }
};

/// \brief Half-open `[start, end)` byte range inside a specific source.
///
/// `Span` is the diagnostics-layer counterpart to `lexer::Range`. The two
/// types are kept separate on purpose: `Range` is the lexer's source-less
/// byte range (a token doesn't know which source it came from), while
/// `Span` adds the `SourceId` so a diagnostic can point back at the right
/// place. Convert with `Span{sourceId, range.start, range.end}` when
/// crossing the boundary.
///
/// `start` and `end` are byte offsets into the text the source was
/// registered with. For a single-character location, set `end = start + 1`.
struct [[nodiscard]] Span final {
  SourceId source;
  uint32_t start = 0;
  uint32_t end = 0;

  /// \brief Width of the span in bytes.
  [[nodiscard]] uint32_t length() const { return end - start; }

  /// \brief True if the span covers no bytes (e.g. a synthetic point span).
  [[nodiscard]] bool empty() const { return start == end; }

  bool operator==(const Span &other) const {
    return source == other.source && start == other.start && end == other.end;
  }
  bool operator!=(const Span &other) const { return !(*this == other); }
};

} // namespace yuzu::diagnostics

#endif // YUZU_DIAGNOSTICS_SPAN_H
