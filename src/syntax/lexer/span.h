#ifndef ORION_SYNTAX_LEXER_SPAN_H_
#define ORION_SYNTAX_LEXER_SPAN_H_

namespace orion::syntax {

/**
 * @brief Represents a span (range) within a source text.
 *
 * The `Span` class defines a start and an end position, typically used to
 * track ranges within a text, such as token positions in a lexer.
 */
class Span {
 public:
  /**
   * @brief Constructs a `Span` with the given start and end positions.
   *
   * @param start The starting position of the span (inclusive).
   * @param end The ending position of the span (exclusive).
   *
   * @note The constructor is explicit to prevent unintended implicit
   * conversions.
   */
  explicit Span(const size_t start, const size_t end)
      : start_(start), end_(end) {}

  /**
   * @brief Deleted default constructor.
   *
   * `Span` objects must be explicitly constructed with a start and end value.
   */
  Span() = delete;

  /**
   * @brief Returns the starting position of the span.
   *
   * @return The start position (inclusive).
   */
  [[nodiscard]] size_t Start() const { return start_; }

  /**
   * @brief Returns the ending position of the span.
   *
   * @return The end position (exclusive).
   */
  [[nodiscard]] size_t End() const { return end_; }

  /**
   * @brief Compares two `Span` objects for equality.
   *
   * @param other The other `Span` to compare with.
   * @return `true` if both spans have the same start and end values, otherwise
   * `false`.
   */
  bool operator==(const Span &other) const {
    return start_ == other.start_ && end_ == other.end_;
  }

 private:
  /** The starting position of the span (inclusive). */
  const size_t start_;
  
  /** The ending position of the span (exclusive). */
  const size_t end_;
};
}  // namespace orion::syntax
#endif  // ORION_SYNTAX_LEXER_SPAN_H_