#ifndef UTIL_ERROR_HANDLING_H
#define UTIL_ERROR_HANDLING_H

#include <cstdlib>
// #include <fmt/core.h>

namespace yuzu::util {
// [[noreturn]] void yuzu_unreachable(const char *msg = nullptr,
//                                    const char *file = nullptr,
//                                    unsigned line = 0) {
[[noreturn]] inline void yuzu_unreachable() {
  //   fmt::println("");
  std::abort();
}
} // namespace yuzu::util

#endif // UTIL_ERROR_HANDLING_H
