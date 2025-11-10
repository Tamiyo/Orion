#ifndef YUZU_LEXER_SPAN_H
#define YUZU_LEXER_SPAN_H

#include <cstdint>

namespace yuzu::lexer {
/// \brief Represents a range of positions in the source text.
///
/// Range defines a half-open interval [start, end) in the source text,
/// where start is inclusive and end is exclusive. This is used to track
/// the location of tokens in the original source code.
struct Range final {
  /// The starting position (inclusive) in the source text.
  const uint32_t start;

  /// The ending position (exclusive) in the source text.
  const uint32_t end;

  /// \brief Equality comparison operator.
  ///
  /// \param other The Range to compare with.
  /// \return True if both ranges have the same start and end positions.
  bool operator==(const Range &other) const {
    return start == other.start && end == other.end;
  }
};
} // namespace yuzu::lexer

#endif // YUZU_LEXER_SPAN_H
