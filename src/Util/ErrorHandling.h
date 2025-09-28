#ifndef UTIL_ERROR_HANDLING_H
#define UTIL_ERROR_HANDLING_H

// #include <fmt/core.h>
#include <utility>

namespace yuzu::util {
// [[noreturn]] void yuzu_unreachable(const char *msg = nullptr,
//                                    const char *file = nullptr,
//                                    unsigned line = 0) {
[[noreturn]] inline void yuzu_unreachable() { std::abort(); }
} // namespace yuzu::util

#endif // UTIL_ERROR_HANDLING_H
