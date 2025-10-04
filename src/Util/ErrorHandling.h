#ifndef UTIL_ERROR_HANDLING_H
#define UTIL_ERROR_HANDLING_H

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

#endif // UTIL_ERROR_HANDLING_H
