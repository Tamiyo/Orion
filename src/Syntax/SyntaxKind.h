#ifndef SYNTAX_SYNTAX_KIND_H
#define SYNTAX_SYNTAX_KIND_H

#include <cstdint>

namespace yuzu::syntax {
using SyntaxKind = uint16_t;

template <typename ExternalKind> class SyntaxKindConverter {
public:
  SyntaxKindConverter() = delete;

  virtual SyntaxKind toInternal(const ExternalKind &Kind);
  virtual ExternalKind fromInternal(const SyntaxKind &Kind);
};
} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAX_KIND_H
