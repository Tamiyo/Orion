#include "yuzu/Diagnostics/SourceMap.h"

#include "yuzu/Diagnostics/Span.h"

#include <gtest/gtest.h>

namespace {
using yuzu::diagnostics::LineCol;
using yuzu::diagnostics::SourceId;
using yuzu::diagnostics::SourceMap;

TEST(SourceMapTest, AddAssignsDistinctIds) {
  SourceMap sm;
  const SourceId a = sm.add("<a>", "x");
  const SourceId b = sm.add("<b>", "y");

  EXPECT_NE(a, b);
  // The default-constructed sentinel is reserved.
  EXPECT_NE(SourceId{}, a);
  EXPECT_NE(SourceId{}, b);
}

TEST(SourceMapTest, GetNameAndTextRoundTrip) {
  SourceMap sm;
  const SourceId id = sm.add("src/main.yuzu", "hello world");

  EXPECT_EQ("src/main.yuzu", sm.getName(id));
  EXPECT_EQ("hello world", sm.getText(id));
}

TEST(SourceMapTest, GetLineColForSingleLine) {
  SourceMap sm;
  const SourceId id = sm.add("<test>", "hello");

  EXPECT_EQ((LineCol{1, 1}), sm.getLineCol(id, 0));
  EXPECT_EQ((LineCol{1, 2}), sm.getLineCol(id, 1));
  EXPECT_EQ((LineCol{1, 5}), sm.getLineCol(id, 4));
  // One-past-the-end is a valid query: it's where end-of-input
  // diagnostics point.
  EXPECT_EQ((LineCol{1, 6}), sm.getLineCol(id, 5));
}

TEST(SourceMapTest, GetLineColAcrossNewlines) {
  SourceMap sm;
  //                         offsets:
  //                         0..3:   "abc"
  //                         3:      '\n'
  //                         4..6:   "de"
  //                         6:      '\n'
  //                         7..9:   "fgh"
  const SourceId id = sm.add("<test>", "abc\nde\nfgh");

  EXPECT_EQ((LineCol{1, 1}), sm.getLineCol(id, 0));   // 'a'
  EXPECT_EQ((LineCol{1, 4}), sm.getLineCol(id, 3));   // '\n' on line 1
  EXPECT_EQ((LineCol{2, 1}), sm.getLineCol(id, 4));   // 'd'
  EXPECT_EQ((LineCol{2, 2}), sm.getLineCol(id, 5));   // 'e'
  EXPECT_EQ((LineCol{2, 3}), sm.getLineCol(id, 6));   // '\n' on line 2
  EXPECT_EQ((LineCol{3, 1}), sm.getLineCol(id, 7));   // 'f'
  EXPECT_EQ((LineCol{3, 4}), sm.getLineCol(id, 10));  // EOF
}

TEST(SourceMapTest, GetLineTextStripsTrailingNewline) {
  SourceMap sm;
  const SourceId id = sm.add("<test>", "abc\nde\nfgh");

  EXPECT_EQ("abc", sm.getLineText(id, 1));
  EXPECT_EQ("de", sm.getLineText(id, 2));
  EXPECT_EQ("fgh", sm.getLineText(id, 3));
}

TEST(SourceMapTest, GetLineTextStripsCarriageReturn) {
  // Windows-style line endings. The renderer doesn't want the trailing
  // `\r` either; otherwise the caret column count gets off by one.
  SourceMap sm;
  const SourceId id = sm.add("<test>", "abc\r\ndef\r\n");

  EXPECT_EQ("abc", sm.getLineText(id, 1));
  EXPECT_EQ("def", sm.getLineText(id, 2));
}

TEST(SourceMapTest, GetLineTextOutOfRangeReturnsEmpty) {
  SourceMap sm;
  const SourceId id = sm.add("<test>", "single");

  EXPECT_EQ("", sm.getLineText(id, 0));
  EXPECT_EQ("", sm.getLineText(id, 2));
}

TEST(SourceMapTest, EmptySourceHasOneEmptyLine) {
  SourceMap sm;
  const SourceId id = sm.add("<test>", "");

  EXPECT_EQ((LineCol{1, 1}), sm.getLineCol(id, 0));
  EXPECT_EQ("", sm.getLineText(id, 1));
}

TEST(SourceMapTest, TrailingNewlineProducesEmptyTrailingLine) {
  // A file ending in `\n` is conventionally still N lines, but the
  // mapping produces an addressable line N+1 (empty) since the next
  // character would land there. This matches editor "go to line"
  // behaviour and avoids special-casing in the renderer.
  SourceMap sm;
  const SourceId id = sm.add("<test>", "abc\n");

  EXPECT_EQ("abc", sm.getLineText(id, 1));
  EXPECT_EQ("", sm.getLineText(id, 2));
  EXPECT_EQ((LineCol{2, 1}), sm.getLineCol(id, 4));
}

TEST(SourceMapTest, MultipleSourcesAreIndependent) {
  SourceMap sm;
  const SourceId a = sm.add("<a>", "abc");
  const SourceId b = sm.add("<b>", "longer\ntext");

  EXPECT_EQ("abc", sm.getText(a));
  EXPECT_EQ("longer\ntext", sm.getText(b));
  EXPECT_EQ((LineCol{1, 4}), sm.getLineCol(a, 3));
  EXPECT_EQ((LineCol{2, 5}), sm.getLineCol(b, 11));
}

} // namespace
