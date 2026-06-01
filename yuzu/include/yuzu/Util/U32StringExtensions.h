#ifndef YUZU_UTIL_U32STRING_H
#define YUZU_UTIL_U32STRING_H

#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/Hashing.h>

#include <cstdlib>
#include <string_view>

namespace llvm {

/// `DenseMapInfo` for `std::u32string_view` (LLVM ships one only for
/// `StringRef`). Mirrors that: sentinel keys by data pointer, real keys by
/// content.
template <> struct DenseMapInfo<std::u32string_view> {
  static inline std::u32string_view getEmptyKey() {
    return std::u32string_view(
        reinterpret_cast<const char32_t *>(~static_cast<uintptr_t>(0)), 0);
  }

  static inline std::u32string_view getTombstoneKey() {
    return std::u32string_view(
        reinterpret_cast<const char32_t *>(~static_cast<uintptr_t>(1)), 0);
  }

  static unsigned getHashValue(std::u32string_view val) {
    return static_cast<unsigned>(hash_combine_range(val.begin(), val.end()));
  }

  static bool isEqual(std::u32string_view lhs, std::u32string_view rhs) {
    if (rhs.data() == getEmptyKey().data()) {
      return lhs.data() == getEmptyKey().data();
    }
    if (rhs.data() == getTombstoneKey().data()) {
      return lhs.data() == getTombstoneKey().data();
    }
    return lhs == rhs;
  }
};

} // namespace llvm

#endif // YUZU_UTIL_ERROR_HANDLING_H
