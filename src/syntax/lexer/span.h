#ifndef ORION_SYNTAX_LEXER_SPAN_H_
#define ORION_SYNTAX_LEXER_SPAN_H_

namespace orion::syntax {
class Span {
 public:
  Span(const size_t start, const size_t end) : start_(start), end_(end) {}

  [[nodiscard]] size_t GetStart() const { return start_; }
  [[nodiscard]] size_t GetEnd() const { return end_; }

  bool operator==(const Span &other) const {
    return start_ == other.start_ && end_ == other.end_;
  }

 private:
  size_t start_;
  size_t end_;
};
}  // namespace orion::syntax
#endif  // ORION_SYNTAX_LEXER_SPAN_H_