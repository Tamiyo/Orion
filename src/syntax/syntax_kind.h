#ifndef SYNTAX_SYNTAX_KIND_H_
#define SYNTAX_SYNTAX_KIND_H_

#include <cstdint>
#include <functional>

namespace yuzu::syntax {
struct SyntaxKind {
  uint16_t value;

  bool operator==(const SyntaxKind& other) const {
    return value == other.value;
  }
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_SYNTAX_KIND_H_
