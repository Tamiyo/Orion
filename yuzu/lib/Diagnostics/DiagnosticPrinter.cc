#include "yuzu/Diagnostics/DiagnosticPrinter.h"

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <cstdint>
#include <string>

namespace yuzu::diagnostics {

namespace {

/// Word printed before the optional `[code]` and the message body.
const char *severityName(Severity s) {
  switch (s) {
  case Severity::Error:
    return "error";
  case Severity::Warning:
    return "warning";
  case Severity::Remark:
    return "remark";
  }
  return "error";
}

/// Width, in characters, of `line` rendered as a decimal line number.
/// The gutter's `|` rows are indented by this many spaces so they align
/// under the `|` that follows the line number on the snippet row.
unsigned lineNumberWidth(uint32_t line) {
  unsigned w = 1;
  while (line >= 10) {
    line /= 10;
    w += 1;
  }
  return w;
}

/// Print `n` spaces. `raw_ostream::indent` uses an `unsigned int` count
/// in our LLVM build; use it directly to avoid local loops.
void indent(llvm::raw_ostream &out, unsigned n) { out.indent(n); }

/// Underline character for a label of the given style.
char underlineFor(LabelStyle style) {
  return style == LabelStyle::Primary ? '^' : '-';
}

} // namespace

void DiagnosticPrinter::print(const Diagnostic &diagnostic,
                              llvm::raw_ostream &out) const {
  // Header line: `severity[CODE]: message`.
  out << severityName(diagnostic.severity);
  if (!diagnostic.code.empty()) {
    out << '[' << diagnostic.code << ']';
  }
  out << ": " << diagnostic.message << '\n';

  // The first Primary label drives the location pointer (`--> name:l:c`)
  // and the snippet line. Other labels render below as additional
  // underlines on the same line; cross-line additional labels are
  // currently rendered without their own snippets (TODO).
  const Label *primary = nullptr;
  for (const Label &l : diagnostic.labels) {
    if (l.style == LabelStyle::Primary) {
      primary = &l;
      break;
    }
  }

  if (primary != nullptr) {
    const SourceId src = primary->span.source;
    const LineCol pos = sources.getLineCol(src, primary->span.start);
    const std::string_view name = sources.getName(src);

    // The arrow line is fixed at one space of indent (matches rustc):
    // `<space>--> name:line:col`. The gutter `|` rows are indented by
    // the line-number width so their `|` aligns with the `|` that
    // separates the line number from the snippet (e.g. `100 | code` ↔
    // `    |`).
    const unsigned gutterIndent = lineNumberWidth(pos.line);

    // Location line.
    out << " --> " << name << ':' << pos.line << ':' << pos.column << '\n';

    // Empty gutter row above the snippet for visual breathing room.
    indent(out, gutterIndent);
    out << " |\n";

    // The snippet line itself, with the line number on the left.
    // The line is UTF-32; encode it to UTF-8 at write time.
    const std::u32string_view lineText = sources.getLineText(src, pos.line);
    out << pos.line << " | ";
    util::writeUtf8(out, lineText);
    out << '\n';

    // Underline row. Walk the labels and emit a caret at each label's
    // start column; only labels on the same source+line as the primary
    // are drawn here (others are dropped for now).
    indent(out, gutterIndent);
    out << " | ";

    // Build the underline by stepping column-by-column through the
    // primary line. Columns count UTF-32 code points (one per char32_t)
    // — same convention as `getLineCol` — so this is plain ASCII despite
    // the source potentially containing non-ASCII characters. Track
    // which label owns each column so primaries override secondaries.
    std::string underline(lineText.size(), ' ');
    const Label *trailingLabel = nullptr;

    for (const Label &l : diagnostic.labels) {
      if (l.span.source != src) {
        continue;
      }
      const LineCol lp = sources.getLineCol(src, l.span.start);
      if (lp.line != pos.line) {
        continue;
      }
      // 1-indexed column → 0-indexed underline offset.
      const unsigned col = lp.column - 1;
      const unsigned len =
          std::max(1u, static_cast<unsigned>(l.span.end - l.span.start));
      for (unsigned i = 0; i < len && col + i < underline.size(); ++i) {
        // Don't overwrite a primary's caret with a secondary's dash.
        if (underline[col + i] == '^') {
          continue;
        }
        underline[col + i] = underlineFor(l.style);
      }
      // Use the primary's message as the trailing label if present.
      if (l.style == LabelStyle::Primary && trailingLabel == nullptr) {
        trailingLabel = &l;
      }
    }

    // Trim trailing spaces — the buffer was sized to `lineText` but the
    // labels usually only cover a small portion of it.
    while (!underline.empty() && underline.back() == ' ') {
      underline.pop_back();
    }
    out << underline;
    if (trailingLabel != nullptr && !trailingLabel->message.empty()) {
      out << ' ' << trailingLabel->message;
    }
    out << '\n';

    // Per-label message rows, rustc-style. Secondary labels with their
    // own messages render below the combined underline:
    //
    //   1 | 2.1 + 2
    //     | ^^^^^^^
    //     | |     |          <- connector row
    //     | |     - right operand has type `int`
    //     | --- left operand has type `float`
    //
    // The connector row marks each pending label's start column with a
    // `|`. Per-label rows render rightmost-first; labels still pending
    // (to the left) keep their `|` connector until rendered.
    std::vector<const Label *> stacked;
    for (const Label &l : diagnostic.labels) {
      if (l.style == LabelStyle::Primary) {
        continue;
      }
      if (l.message.empty() || l.span.source != src) {
        continue;
      }
      if (sources.getLineCol(src, l.span.start).line != pos.line) {
        continue;
      }
      stacked.push_back(&l);
    }
    std::sort(stacked.begin(), stacked.end(),
              [](const Label *a, const Label *b) {
                return a->span.start < b->span.start;
              });

    if (!stacked.empty()) {
      std::vector<unsigned> cols;
      cols.reserve(stacked.size());
      for (const Label *l : stacked) {
        cols.push_back(sources.getLineCol(src, l->span.start).column - 1);
      }

      // Connector row: `|` at each pending label's start column.
      indent(out, gutterIndent);
      out << " | ";
      std::string connectors(cols.back() + 1, ' ');
      for (unsigned c : cols) {
        connectors[c] = '|';
      }
      out << connectors << '\n';

      // Per-label rows, rightmost first. For each, place `|` connectors
      // at every label to its left (still pending) and draw the current
      // label's underline + message.
      for (std::size_t k = stacked.size(); k-- > 0;) {
        const Label &l = *stacked[k];
        const unsigned col = cols[k];
        const unsigned len =
            std::max(1u, static_cast<unsigned>(l.span.end - l.span.start));
        indent(out, gutterIndent);
        out << " | ";
        std::string row(col, ' ');
        for (std::size_t j = 0; j < k; ++j) {
          row[cols[j]] = '|';
        }
        row.append(len, underlineFor(l.style));
        out << row << ' ' << l.message << '\n';
      }
    }
  }

  // Notes: same gutter indent as the snippet rows, but with `=` instead
  // of `|` so they read as off-tree annotations rather than continuations
  // of the snippet itself.
  const unsigned noteIndent =
      primary != nullptr
          ? lineNumberWidth(
                sources.getLineCol(primary->span.source, primary->span.start)
                    .line)
          : 1;
  for (const std::string &note : diagnostic.notes) {
    indent(out, noteIndent);
    out << " = note: " << note << '\n';
  }
}

std::string
DiagnosticPrinter::printToString(const Diagnostic &diagnostic) const {
  std::string out;
  llvm::raw_string_ostream os(out);
  print(diagnostic, os);
  return out;
}

} // namespace yuzu::diagnostics
