#ifndef UTIL_UTF8_STRING_UTILS_H_
#define UTIL_UTF8_STRING_UTILS_H_

#include <string>

// std::vector<uint32_t> utf8_to_codepoints(const std::string& utf8) {
//   std::vector<uint32_t> codepoints;
//   size_t i = 0;
//
//   while (i < utf8.size()) {
//     uint32_t codepoint = 0;
//     unsigned char byte = utf8[i];
//     size_t num_bytes = 0;
//
//     if ((byte & 0x80) == 0) {
//       // 1-byte character (ASCII)
//       codepoint = byte;
//       num_bytes = 1;
//     } else if ((byte & 0xE0) == 0xC0) {
//       // 2-byte character
//       codepoint = byte & 0x1F;
//       num_bytes = 2;
//     } else if ((byte & 0xF0) == 0xE0) {
//       // 3-byte character
//       codepoint = byte & 0x0F;
//       num_bytes = 3;
//     } else if ((byte & 0xF8) == 0xF0) {
//       // 4-byte character
//       codepoint = byte & 0x07;
//       num_bytes = 4;
//     } else {
//       // Invalid UTF-8
//       ++i;
//       continue;
//     }
//
//     // Decode the rest of the bytes
//     for (size_t j = 1; j < num_bytes; ++j) {
//       if (i + j >= utf8.size() || (utf8[i + j] & 0xC0) != 0x80) {
//         // Invalid continuation byte
//         codepoint = '?'; // Replacement character
//         break;
//       }
//       codepoint = (codepoint << 6) | (utf8[i + j] & 0x3F);
//     }
//
//     codepoints.push_back(codepoint);
//     i += num_bytes;
//   }
//
//   return codepoints;
// }

// https://unicode-org.github.io/icu/userguide/icu/unicode.html
// https://alx71hub.github.io/hcb/#universal-character-name
// https://chatgpt.com/c/67ce74df-dbe0-800e-807c-a2497ca9eeb5
namespace orion::util {
class Utf8String {
 public:
  explicit Utf8String(const std::string& string) : string_(string) {}

 private:
  std::string string_;
};
}  // namespace orion::util

#endif  // UTIL_UTF8_STRING_UTILS_H_
