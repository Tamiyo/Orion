#include "yuzu/Diagnostics/DiagnosticPrinter.h"

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/SourceMap.h"
#include "yuzu/Diagnostics/Span.h"

#include <gtest/gtest.h>

namespace {
using yuzu::diagnostics::DiagnosticPrinter;
using yuzu::diagnostics::DiagnosticsEngine;
using yuzu::diagnostics::SourceId;
using yuzu::diagnostics::SourceMap;
using yuzu::diagnostics::Span;

class DiagnosticPrinterTest : public ::testing::Test {
protected:
  SourceMap sources;
  DiagnosticsEngine engine;

  Span span(SourceId src, uint32_t start, uint32_t end) {
    return Span{src, start, end};
  }
};

TEST_F(DiagnosticPrinterTest, RendersSimpleErrorWithCaret) {
  const SourceId src = sources.add("<repl>", U"2 ^ 2");
  engine.error(span(src, 2, 3), "expected expression").emit();

  const DiagnosticPrinter printer(sources);
  EXPECT_EQ(R"(error: expected expression
 --> <repl>:1:3
  |
1 | 2 ^ 2
  |   ^
)",
            printer.printToString(engine.getDiagnostics()[0]));
}

TEST_F(DiagnosticPrinterTest, IncludesCodeInHeader) {
  const SourceId src = sources.add("<repl>", U"2 ^ 2");
  engine.error(span(src, 2, 3), "expected expression").code("E0001").emit();

  const DiagnosticPrinter printer(sources);
  EXPECT_EQ(R"(error[E0001]: expected expression
 --> <repl>:1:3
  |
1 | 2 ^ 2
  |   ^
)",
            printer.printToString(engine.getDiagnostics()[0]));
}

TEST_F(DiagnosticPrinterTest, AppendsPrimaryLabelMessage) {
  const SourceId src = sources.add("<repl>", U"2 ^ 2");
  // Replace the seeded primary label with one that carries a message,
  // so the renderer prints "^ expected expression".
  engine.error(span(src, 2, 3), "expected expression")
      .primaryLabel(span(src, 2, 3), "expected expression")
      .emit();

  const DiagnosticPrinter printer(sources);
  // Two primaries; the first carries the trailing label text.
  EXPECT_EQ(R"(error: expected expression
 --> <repl>:1:3
  |
1 | 2 ^ 2
  |   ^
)",
            printer.printToString(engine.getDiagnostics()[0]));
}

TEST_F(DiagnosticPrinterTest, RendersSecondaryLabelOnSameLine) {
  const SourceId src = sources.add("<repl>", U"1 + +");
  // Primary at the trailing `+`; secondary at the leading `+` for
  // context. Same line, so both underlines stack.
  engine.error(span(src, 4, 5), "expected expression")
      .label(span(src, 2, 3), "after this `+`")
      .emit();

  const DiagnosticPrinter printer(sources);
  EXPECT_EQ(R"(error: expected expression
 --> <repl>:1:5
  |
1 | 1 + +
  |   - ^
)",
            printer.printToString(engine.getDiagnostics()[0]));
}

TEST_F(DiagnosticPrinterTest, AppendsNotesUnderSnippet) {
  const SourceId src = sources.add("<repl>", U"2 ^ 2");
  engine.error(span(src, 2, 3), "expected expression")
      .note("expressions can start with a number, identifier, or `(`")
      .emit();

  const DiagnosticPrinter printer(sources);
  EXPECT_EQ(R"(error: expected expression
 --> <repl>:1:3
  |
1 | 2 ^ 2
  |   ^
  = note: expressions can start with a number, identifier, or `(`
)",
            printer.printToString(engine.getDiagnostics()[0]));
}

TEST_F(DiagnosticPrinterTest, ReportsLineColForLaterLine) {
  // Multi-line input: the renderer should pick up the line containing
  // the primary span and align the gutter to its line-number width.
  const SourceId src = sources.add("<test>", U"first\nsecond\nthird");
  // Span over 'c' in "second" (offset 8) — line 2, column 3.
  engine.error(span(src, 8, 9), "look here").emit();

  const DiagnosticPrinter printer(sources);
  EXPECT_EQ(R"(error: look here
 --> <test>:2:3
  |
2 | second
  |   ^
)",
            printer.printToString(engine.getDiagnostics()[0]));
}

TEST_F(DiagnosticPrinterTest, SeverityWordTracksDiagnosticKind) {
  const SourceId src = sources.add("<repl>", U"x");
  engine.warning(span(src, 0, 1), "unused").emit();

  const DiagnosticPrinter printer(sources);
  EXPECT_EQ(R"(warning: unused
 --> <repl>:1:1
  |
1 | x
  | ^
)",
            printer.printToString(engine.getDiagnostics()[0]));
}

} // namespace
