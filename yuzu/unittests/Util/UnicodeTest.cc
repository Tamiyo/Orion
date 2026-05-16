#include "yuzu/Util/Unicode.h"

#include <llvm/Support/raw_ostream.h>

#include <gtest/gtest.h>

#include <string>

namespace {

std::string encode(char32_t c) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::util::writeUtf8(os, c);
  return out;
}

std::string encode(std::u32string_view text) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::util::writeUtf8(os, text);
  return out;
}

std::string escape(std::u32string_view text) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::util::writeEscapedQuoted(os, text);
  return out;
}

TEST(UnicodeTest, EncodesAsciiAsSingleByte) {
  EXPECT_EQ(std::string("\x00", 1), encode(U'\0'));
  EXPECT_EQ("A", encode(U'A'));
  EXPECT_EQ("~", encode(U'~'));
  // U+007F is the largest single-byte code point.
  EXPECT_EQ("\x7F", encode(U''));
}

TEST(UnicodeTest, EncodesTwoByteBoundaries) {
  // U+0080 is the smallest two-byte code point.
  EXPECT_EQ("\xC2\x80", encode(U''));
  // U+00E9 (é): typical Latin-1 supplement code point.
  EXPECT_EQ("\xC3\xA9", encode(U'é'));
  // U+07FF is the largest two-byte code point.
  EXPECT_EQ("\xDF\xBF", encode(U'߿'));
}

TEST(UnicodeTest, EncodesThreeByteBoundaries) {
  // U+0800 is the smallest three-byte code point.
  EXPECT_EQ("\xE0\xA0\x80", encode(U'ࠀ'));
  // U+4E2D (中): mid-BMP CJK character.
  EXPECT_EQ("\xE4\xB8\xAD", encode(U'中'));
  // U+FFFF is the largest three-byte code point (and the BMP cap).
  EXPECT_EQ("\xEF\xBF\xBF", encode(U'￿'));
}

TEST(UnicodeTest, EncodesFourByteBoundaries) {
  // U+10000 is the smallest four-byte code point.
  EXPECT_EQ("\xF0\x90\x80\x80", encode(U'\U00010000'));
  // U+1F600 (😀): grinning face emoji.
  EXPECT_EQ("\xF0\x9F\x98\x80", encode(U'\U0001F600'));
  // U+10FFFF is the largest valid Unicode code point.
  EXPECT_EQ("\xF4\x8F\xBF\xBF", encode(U'\U0010FFFF'));
}

TEST(UnicodeTest, StringFormConcatenatesCodePoints) {
  EXPECT_EQ("", encode(std::u32string_view()));
  EXPECT_EQ("hi", encode(U"hi"));
  // Mixed: ASCII + 2-byte + 3-byte + 4-byte.
  EXPECT_EQ("a\xC3\xA9\xE4\xB8\xAD\xF0\x9F\x98\x80", encode(U"aé中\U0001F600"));
}

TEST(UnicodeTest, EscapesBackslashAndQuote) {
  EXPECT_EQ(R"(\\)", escape(U"\\"));
  EXPECT_EQ(R"(\")", escape(U"\""));
  EXPECT_EQ(R"(a\\b\"c)", escape(U"a\\b\"c"));
}

TEST(UnicodeTest, EscapesControlCharacters) {
  EXPECT_EQ(R"(\n)", escape(U"\n"));
  EXPECT_EQ(R"(\r)", escape(U"\r"));
  EXPECT_EQ(R"(\t)", escape(U"\t"));
  EXPECT_EQ(R"(\n\r\t)", escape(U"\n\r\t"));
}

TEST(UnicodeTest, LeavesEmptyStringEmpty) {
  EXPECT_EQ("", escape(std::u32string_view()));
}

TEST(UnicodeTest, PassesThroughOrdinaryCharacters) {
  EXPECT_EQ("hello world", escape(U"hello world"));
}

TEST(UnicodeTest, EscapedOutputIsUtf8ForNonAscii) {
  // Non-special non-ASCII code points pass through as UTF-8 alongside
  // escaped specials. Input: `é"\` (3 code points). Output bytes: the
  // 2-byte UTF-8 for é, followed by `\"` (escaped quote), followed by
  // `\\` (escaped backslash).
  EXPECT_EQ("\xC3\xA9\\\"\\\\", escape(U"é\"\\"));
}

} // namespace
