#ifndef YUZU_UTIL_STRINGINTERNER_H
#define YUZU_UTIL_STRINGINTERNER_H

#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/Support/Allocator.h>

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <unordered_set>

namespace yuzu::util {

/// Deduplicating arena for strings. Equal content interns to one stable
/// `std::u32string_view`, comparable by `data()` pointer.
class StringInterner final {
public:
  StringInterner() = default;
  StringInterner(const StringInterner &) = delete;
  StringInterner &operator=(const StringInterner &) = delete;

  /// Intern `s` to a stable view. Empty strings don't allocate.
  std::u32string_view intern(std::u32string_view s) {
    if (s.empty()) {
      return {};
    }             
    if (const auto it = pool.find(s); it != pool.end()) {
      return *it;
    }
    auto *buf  = arena.Allocate<char32_t>(s.size());
    std::copy(s.begin(), s.end(), buf);
    const std::u32string_view interned(buf, s.size());
    pool.insert(interned);
    return interned;
  }

private:
  llvm::BumpPtrAllocator arena;
  // Stores only arena-backed views; transient lookup keys match by content.
  std::unordered_set<std::u32string_view> pool;
};

} // namespace yuzu::util

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

#endif // YUZU_UTIL_STRINGINTERNER_H
