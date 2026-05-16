#ifndef YUZU_UTIL_ERROR_HANDLING_H
#define YUZU_UTIL_ERROR_HANDLING_H

#include <llvm/Support/raw_ostream.h>

#include <cstdlib>

namespace yuzu::util {
[[noreturn]] inline void yuzu_unreachable(const char *msg = nullptr) {
  if (msg) {
    llvm::errs() << "unreachable reached: " << msg << '\n';
  }

  std::abort();
}
} // namespace yuzu::util

#endif // YUZU_UTIL_ERROR_HANDLING_H
