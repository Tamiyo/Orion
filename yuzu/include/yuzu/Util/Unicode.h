#ifndef YUZU_UTIL_UNICODE_H
#define YUZU_UTIL_UNICODE_H

#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <string_view>

namespace yuzu::util {

/// \brief Write a single Unicode code point as UTF-8 to `os`.
///
/// Invalid code points (surrogates, values above U+10FFFF) are encoded as
/// if valid; the caller is expected to provide well-formed input.
///
/// \param os The stream to write to.
/// \param c The code point to encode.
inline void writeUtf8(llvm::raw_ostream &os, char32_t c) {
  const auto u = static_cast<uint32_t>(c);
  if (u < 0x80) {
    os << static_cast<char>(u);
  } else if (u < 0x800) {
    os << static_cast<char>(0xC0 | (u >> 6));
    os << static_cast<char>(0x80 | (u & 0x3F));
  } else if (u < 0x10000) {
    os << static_cast<char>(0xE0 | (u >> 12));
    os << static_cast<char>(0x80 | ((u >> 6) & 0x3F));
    os << static_cast<char>(0x80 | (u & 0x3F));
  } else {
    os << static_cast<char>(0xF0 | (u >> 18));
    os << static_cast<char>(0x80 | ((u >> 12) & 0x3F));
    os << static_cast<char>(0x80 | ((u >> 6) & 0x3F));
    os << static_cast<char>(0x80 | (u & 0x3F));
  }
}

/// \brief Write a UTF-32 string as UTF-8 to `os`.
///
/// \param os The stream to write to.
/// \param text The text to encode.
inline void writeUtf8(llvm::raw_ostream &os, std::u32string_view text) {
  for (char32_t c : text) {
    writeUtf8(os, c);
  }
}

/// \brief Write a UTF-32 string as UTF-8 to `os`, escaping characters that
/// are problematic when embedded in a double-quoted literal.
///
/// The caller is responsible for emitting the surrounding quotes; this
/// helper escapes only the body. Specifically: backslash, double-quote,
/// newline, carriage return, and tab become their two-character `\\X`
/// equivalents. Everything else is encoded as UTF-8.
///
/// \param os The stream to write to.
/// \param text The text to encode.
inline void writeEscapedQuoted(llvm::raw_ostream &os,
                               std::u32string_view text) {
  for (char32_t c : text) {
    switch (c) {
    case U'\\':
      os << "\\\\";
      continue;
    case U'"':
      os << "\\\"";
      continue;
    case U'\n':
      os << "\\n";
      continue;
    case U'\r':
      os << "\\r";
      continue;
    case U'\t':
      os << "\\t";
      continue;
    default:
      writeUtf8(os, c);
    }
  }
}

} // namespace yuzu::util

#endif // YUZU_UTIL_UNICODE_H
