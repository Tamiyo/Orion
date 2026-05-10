#ifndef YUZU_DIAGNOSTICS_DIAGNOSTIC_H
#define YUZU_DIAGNOSTICS_DIAGNOSTIC_H

#include <cstdint>
#include <string>

namespace yuzu::diagnostics {

enum class Severity : uint8_t { Error, Remark, Warning };

struct Location {};

struct Diagnostic {
  std::string message;
  Location location;
  Severity severity;
};

} // namespace yuzu::diagnostics

#endif // YUZU_DIAGNOSTICS_DIAGNOSTIC_H
