#ifndef SYNTAX_PARSER_PARSER_H_
#define SYNTAX_PARSER_PARSER_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include "syntax/lexer/span.h"
#include "syntax/lexer/token.h"
#include "syntax/parser/error/parse_error.h"
#include "syntax/parser/event.h"
#include "syntax/parser/marker.h"
#include "syntax/parser/token_source.h"

namespace yuzu::syntax {
template <typename TokenKind = uint16_t, typename SyntaxKind = uint16_t>
class Parser {
 private:
  using Event = Event<TokenKind, SyntaxKind>;
  using StartEvent = StartEvent<SyntaxKind>;
  using Token = Token<TokenKind>;
  using TokenSource = TokenSource<TokenKind>;

 public:
  explicit Parser(const TokenSource& source)
      : source_(std::move(source)), events_({}), expected_kinds_({}) {}

  Parser() = delete;
  virtual ~Parser() = default;

  Marker Start() {
    const size_t position = events_.size();
    events_.emplace_back(PlaceholderEvent{});
    return Marker(position);
  }

  Marker Precede(const CompletedMarker completed_marker) {
    const Marker marker = Start();

    Event& event = events_.at(completed_marker.Position());
    StartEvent start = std::get<StartEvent>(event);
    start.SetForwardParent(marker.Position() - completed_marker.Position());

    // TODO(tamiyo) Do we even need to do this?
    events_[completed_marker.Position()] = start;
    return marker;
  }

  CompletedMarker Complete(const Marker marker, SyntaxKind kind) {
    const size_t position = marker.Position();

    if (Event& event = events_.at(position);
        !std::holds_alternative<PlaceholderEvent>(event)) {
      throw std::invalid_argument(
          "cannot complete a marker that isn't a placeholder");
    }

    events_[position] = StartEvent(kind, std::nullopt);
    events_.emplace_back(FinishEvent{});

    return CompletedMarker(position);
  }

  void Expect(const TokenKind kind) noexcept {
    if (At(kind)) {
      Bump();
    } else {
      const std::array<TokenKind, 0> kExprRecoverySet = {};
      Error(kExprRecoverySet);
    }
  }

  void Bump() noexcept {
    expected_kinds_.clear();
    auto _ = source_.NextToken();
    events_.emplace_back(TokenEvent{});
  }

  void BumpMaybe(TokenKind kind) noexcept {
    if (At(kind)) {
      Bump();
    }
  }

  [[nodiscard]] bool At(const TokenKind kind) noexcept {
    return PeekKind() == kind;
  }

  [[nodiscard]] bool AtEnd() noexcept {
    return source_.PeekToken() == std::nullopt;
  }

  [[nodiscard]] std::optional<Token> PeekToken() { return source_.PeekToken(); }

  [[nodiscard]] std::optional<TokenKind> PeekKind() {
    return source_.PeekKind();
  }

  template <std::size_t N>
  void Error(std::array<TokenKind, N> recovery_set) {
    const std::optional<Token> token = PeekToken();

    const std::optional<TokenKind> found =
        token.has_value() ? std::make_optional(token.value().Kind())
                          : std::nullopt;

    const Span span = token.has_value() ? token.value().Span()
                                        : source_.LastTokenSpan().value();

    const ParseError error = ParseError{
        .expected = std::vector(expected_kinds_), .found = found, .span = span};

    events_.emplace_back(ErrorEvent(error));

    if (!AtSet(recovery_set)) {
      const Marker m = Start();
      Bump();
      Complete(m, SyntaxKind::kError);
    }
  }

  const std::vector<Event>& Events() const noexcept { return events_; }

 private:
  template <std::size_t N>
  [[nodiscard]] bool AtSet(std::array<TokenKind, N> recovery_set) {
    if (const auto kind = PeekKind(); kind.has_value()) {
      const TokenKind* found = std::find(std::begin(recovery_set),
                                         std::end(recovery_set), kind.value());
      return found != std::end(recovery_set);
    }

    return false;
  }

  TokenSource source_;
  std::vector<Event> events_;
  std::vector<TokenKind> expected_kinds_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_PARSER_H_
