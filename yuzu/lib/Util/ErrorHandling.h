#ifndef YUZU_UTIL_ERROR_HANDLING_H
#define YUZU_UTIL_ERROR_HANDLING_H

#include <cstdlib>
#include <fmt/core.h>

namespace yuzu::util {
[[noreturn]] inline void yuzu_unreachable(const char *Msg = nullptr) {
  if (Msg) {
    fmt::println("unreachable reached: %s", Msg);
  }

  std::abort();
}
} // namespace yuzu::util

#endif // YUZU_UTIL_ERROR_HANDLING_H
