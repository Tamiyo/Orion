#ifndef UTILS_STRING_UTILS_H_
#define UTILS_STRING_UTILS_H_

#include <stdexcept>
#include <string>

namespace yuzu::utils {
inline std::string U32ToU8(const std::u32string& input) {
  std::string output;
  for (char32_t codepoint : input) {
    if (codepoint <= 0x7F) {
      output.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
      output.push_back(static_cast<char>(0xC0 | (codepoint >> 6)));
      output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
      output.push_back(static_cast<char>(0xE0 | (codepoint >> 12)));
      output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
      output.push_back(static_cast<char>(0xF0 | (codepoint >> 18)));
      output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
      output.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else {
      throw std::runtime_error("Invalid UTF-32 code point");
    }
  }
  return output;
}
}  // namespace yuzu::utils

#endif  // UTILS_STRING_UTILS_H_
