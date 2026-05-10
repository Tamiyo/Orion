#ifndef YUZU_DIAGNOSTICS_SOURCE_MAP_H
#define YUZU_DIAGNOSTICS_SOURCE_MAP_H

#include "yuzu/Diagnostics/Span.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace yuzu::diagnostics {
/// \brief 1-indexed line and column for a byte offset.
///
/// Line and column count Unicode *bytes*, not code points or grapheme
/// clusters. Sufficient for the diagnostic renderer's caret placement on
/// ASCII; non-ASCII alignment is a follow-up problem.
struct [[nodiscard]] LineCol final {
  uint32_t line = 1;
  uint32_t column = 1;

  bool operator==(const LineCol &other) const {
    return line == other.line && column == other.column;
  }
  bool operator!=(const LineCol &other) const { return !(*this == other); }
};

/// \brief Owns source texts and converts byte offsets into them to
/// renderer-friendly forms.
///
/// A `SourceMap` is the source-of-truth for diagnostics rendering: every
/// `SourceId` issued by a `SourceMap` can be resolved back to a name, the
/// raw text, or a `(line, column)` pair for a given byte offset. The
/// driver registers each input (file, REPL line, test fixture, ...) and
/// hands the resulting id to the lexer/parser; diagnostics carry the id
/// and look back here at render time.
///
/// Sources are stored by value, so the `SourceMap` is the lifetime owner
/// — a `Span` is only valid while the `SourceMap` that issued its
/// `SourceId` is alive. Sources cannot be removed once registered (kept
/// simple; reallocation would invalidate held line indices).
class [[nodiscard]] SourceMap final {
public:
  /// \brief Register a new source and return its id.
  ///
  /// `name` is the display label the renderer prints (e.g.
  /// `"src/main.yuzu"`, `"<repl>"`, `"<test>"`); it is not interpreted as
  /// a path. `text` is the raw source body — UTF-8 is expected, but the
  /// SourceMap does not validate.
  ///
  /// \param name Display label.
  /// \param text Source body. Stored by value.
  /// \return A fresh `SourceId` referring to this source.
  SourceId add(std::string name, std::string text);

  /// \brief Display name for a registered source.
  ///
  /// \param id The id, must have been issued by this map.
  /// \return The name passed to `add`.
  [[nodiscard]] std::string_view getName(SourceId id) const;

  /// \brief Raw text for a registered source.
  ///
  /// \param id The id, must have been issued by this map.
  /// \return The text passed to `add`.
  [[nodiscard]] std::string_view getText(SourceId id) const;

  /// \brief Convert a byte offset within a source into 1-indexed
  /// `(line, column)`.
  ///
  /// `offset` may equal the source length (one past the last byte) — that
  /// reports the position immediately after the source, which is what
  /// end-of-input diagnostics want.
  ///
  /// \param id The source id.
  /// \param offset Byte offset within the source's text.
  /// \return The corresponding `LineCol`.
  [[nodiscard]] LineCol getLineCol(SourceId id, uint32_t offset) const;

  /// \brief Slice a single line out of a registered source.
  ///
  /// Returns the bytes of the requested 1-indexed line, excluding the
  /// trailing newline. Used by the renderer to print the source snippet
  /// shown above the caret.
  ///
  /// \param id The source id.
  /// \param line 1-indexed line number.
  /// \return The line's bytes, or an empty view if `line` is out of range.
  [[nodiscard]] std::string_view getLineText(SourceId id, uint32_t line) const;

private:
  /// One registered source plus its precomputed line index.
  ///
  /// `lineStarts` holds the byte offset of the first character of every
  /// line. Line N is `[lineStarts[N - 1], lineStarts[N])` (with a virtual
  /// `lineStarts[count] == text.size()` past the end). Built once at
  /// register time so `getLineCol` is O(log lines).
  struct Entry {
    std::string name;
    std::string text;
    std::vector<uint32_t> lineStarts;
  };

  std::vector<Entry> entries;
};

} // namespace yuzu::diagnostics

#endif // YUZU_DIAGNOSTICS_SOURCE_MAP_H
