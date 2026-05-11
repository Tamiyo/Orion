#include "yuzu/Diagnostics/SourceMap.h"

#include "yuzu/Diagnostics/Span.h"

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace yuzu::diagnostics {

namespace {

/// Build an index of line-start code-point offsets for `text`. The first
/// line always starts at 0; each `\n` introduces a new entry pointing
/// one past the newline. A trailing newline therefore produces an empty
/// trailing line, which matches what users expect when they see "line
/// N" in their editor.
std::vector<uint32_t> indexLines(std::u32string_view text) {
  std::vector<uint32_t> starts;
  starts.push_back(0);
  for (uint32_t i = 0; i < text.size(); ++i) {
    if (text[i] == U'\n') {
      starts.push_back(i + 1);
    }
  }
  return starts;
}

} // namespace

SourceId SourceMap::add(std::string name, std::u32string text) {
  std::vector<uint32_t> starts = indexLines(text);
  entries.push_back(Entry{
      .name = std::move(name),
      .text = std::move(text),
      .lineStarts = std::move(starts),
  });
  // SourceId is 1-indexed so a default-constructed `SourceId{}` (value
  // 0) remains an invalid sentinel callers can't accidentally collide
  // with.
  return SourceId{static_cast<uint32_t>(entries.size())};
}

std::string_view SourceMap::getName(SourceId id) const {
  assert(id.value > 0 && id.value <= entries.size() &&
         "SourceId not issued by this SourceMap");
  return entries[id.value - 1].name;
}

std::u32string_view SourceMap::getText(SourceId id) const {
  assert(id.value > 0 && id.value <= entries.size() &&
         "SourceId not issued by this SourceMap");
  return entries[id.value - 1].text;
}

LineCol SourceMap::getLineCol(SourceId id, uint32_t offset) const {
  assert(id.value > 0 && id.value <= entries.size() &&
         "SourceId not issued by this SourceMap");
  const Entry &entry = entries[id.value - 1];

  // Find the largest lineStarts entry <= offset. `upper_bound` returns
  // the first strictly greater entry; one back is the line containing
  // offset. Always non-empty because `lineStarts` always begins with 0.
  const auto it = std::upper_bound(entry.lineStarts.begin(),
                                   entry.lineStarts.end(), offset);
  const auto lineIndex =
      static_cast<uint32_t>((it - entry.lineStarts.begin()) - 1);

  return LineCol{
      .line = lineIndex + 1,
      .column = (offset - entry.lineStarts[lineIndex]) + 1,
  };
}

std::u32string_view SourceMap::getLineText(SourceId id, uint32_t line) const {
  assert(id.value > 0 && id.value <= entries.size() &&
         "SourceId not issued by this SourceMap");
  const Entry &entry = entries[id.value - 1];

  if (line == 0 || line > entry.lineStarts.size()) {
    return {};
  }

  const uint32_t startOffset = entry.lineStarts[line - 1];
  const uint32_t endOffset = line < entry.lineStarts.size()
                                 ? entry.lineStarts[line]
                                 : static_cast<uint32_t>(entry.text.size());

  // Trim the trailing newline (and an optional `\r` before it) so the
  // renderer doesn't smear the snippet onto two lines.
  uint32_t lineEnd = endOffset;
  if (lineEnd > startOffset && entry.text[lineEnd - 1] == U'\n') {
    --lineEnd;
    if (lineEnd > startOffset && entry.text[lineEnd - 1] == U'\r') {
      --lineEnd;
    }
  }

  return std::u32string_view(entry.text)
      .substr(startOffset, lineEnd - startOffset);
}

} // namespace yuzu::diagnostics
