#ifndef YUZU_UTIL_STRINGINTERNER_H
#define YUZU_UTIL_STRINGINTERNER_H

#include "yuzu/Util/U32StringExtensions.h" // IWYU pragma: keep
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/Allocator.h>

#include <algorithm>
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

  std::u32string_view intern(std::string_view s) {
    return intern(util::decodeUtf8(s));
  }

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

#endif // YUZU_UTIL_STRINGINTERNER_H
