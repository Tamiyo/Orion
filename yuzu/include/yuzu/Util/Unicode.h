#ifndef YUZU_UTIL_UNICODE_H
#define YUZU_UTIL_UNICODE_H

#include "llvm/Support/raw_ostream.h"

#include <cstdint>
#include <string>
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

/// \brief Encode a UTF-32 string as a UTF-8 `std::string`.
///
/// Convenience wrapper around the streaming `writeUtf8` for callers that
/// want the bytes by value rather than written to an `llvm::raw_ostream`
/// (e.g. registering a UTF-32 source with a UTF-8 `SourceMap`).
///
/// \param text The UTF-32 input.
/// \return The UTF-8 encoding.
inline std::string toUtf8(std::u32string_view text) {
  std::string out;
  llvm::raw_string_ostream os(out);
  writeUtf8(os, text);
  return out;
}

/// \brief Decode a UTF-8 byte sequence into a UTF-32 string.
///
/// Inverse of `writeUtf8`. Walks `bytes` left-to-right, emitting one
/// `char32_t` per Unicode code point. Malformed sequences (truncated
/// continuation bytes, stray continuation bytes, oversized leaders) are
/// replaced with U+FFFD (replacement character) one byte at a time so the
/// output length stays bounded by the input length and decoding never
/// raises. Use a strict frontend if you need round-trip fidelity.
///
/// \param bytes The UTF-8 input.
/// \return The decoded UTF-32 string.
inline std::u32string decodeUtf8(std::string_view bytes) {
  std::u32string out;
  out.reserve(bytes.size());

  const auto *p = reinterpret_cast<const unsigned char *>(bytes.data());
  const auto *end = p + bytes.size();

  while (p != end) {
    char32_t c = 0xFFFD;
    if (*p < 0x80) {
      c = *p;
      p += 1;
    } else if ((*p & 0xE0) == 0xC0 && end - p >= 2) {
      c = (static_cast<char32_t>(*p & 0x1F) << 6) |
          static_cast<char32_t>(p[1] & 0x3F);
      p += 2;
    } else if ((*p & 0xF0) == 0xE0 && end - p >= 3) {
      c = (static_cast<char32_t>(*p & 0x0F) << 12) |
          (static_cast<char32_t>(p[1] & 0x3F) << 6) |
          static_cast<char32_t>(p[2] & 0x3F);
      p += 3;
    } else if ((*p & 0xF8) == 0xF0 && end - p >= 4) {
      c = (static_cast<char32_t>(*p & 0x07) << 18) |
          (static_cast<char32_t>(p[1] & 0x3F) << 12) |
          (static_cast<char32_t>(p[2] & 0x3F) << 6) |
          static_cast<char32_t>(p[3] & 0x3F);
      p += 4;
    } else {
      p += 1;
    }
    out.push_back(c);
  }

  return out;
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
