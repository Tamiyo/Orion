#ifndef SYNTAX_PARSER_MARKER_H_
#define SYNTAX_PARSER_MARKER_H_

#include <tuple>
#include <variant>

#include "syntax/parser/parser_base.h"
#include "syntax/syntax_kind.h"

namespace orion::syntax {
class Marker;

class CompletedMarker {
 public:
  explicit CompletedMarker(size_t position) : position_(position) {}

  [[nodiscard]] size_t Position() const noexcept { return this->position_; }

  // TODO(tamiyo) Implement marker support.
  [[nodiscard]] std::tuple<Marker, SyntaxKind> Precede();

 private:
  const size_t position_;
};

class Marker {
 public:
  explicit Marker(size_t position) : position_(position) {}

  [[nodiscard]] size_t Position() const noexcept { return this->position_; }

  // TODO(tamiyo) Implement marker support.
  [[nodiscard]] CompletedMarker Complete();

 private:
  const size_t position_;
};

}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_MARKER_H_
