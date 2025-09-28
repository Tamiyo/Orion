#ifndef SYNTAX_SYNTAX_KIND_H
#define SYNTAX_SYNTAX_KIND_H

#include <cstdint>

namespace yuzu::syntax {
struct SyntaxKind {
  uint16_t Value;

  bool operator==(const SyntaxKind &Other) const {
    return Value == Other.Value;
  }
};

template <typename ExternalKind> class SyntaxKindConverter {
public:
  SyntaxKindConverter() = delete;

  virtual SyntaxKind toInternal(const ExternalKind &Kind);
  virtual ExternalKind fromInternal(const SyntaxKind &Kind);
};
} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_KIND_H
