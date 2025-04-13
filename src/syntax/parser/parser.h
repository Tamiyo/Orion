#ifndef SYNTAX_PARSER_PARSER_H_
#define SYNTAX_PARSER_PARSER_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <stdexcept>
#include <tuple>
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
 public:
  Parser() = delete;
  virtual ~Parser() = default;

 public:
  Marker Start() {
    const size_t position = events_.size();
    events_.emplace_back(PlaceholderEvent{});
    return Marker(position);
  }

  std::tuple<Marker, SyntaxKind> Precede(
      const CompletedMarker completed_marker) {
    const Marker marker = Start();

    const Event<TokenKind, SyntaxKind>& event =
        events_.at(completed_marker.Position());
    StartEvent start = std::get<StartEvent>(event);

    const SyntaxKind prev_kind = start.Kind();
    start.SetForwardParent(marker.Position() - completed_marker.Position());

    return {marker, prev_kind};
  }

  CompletedMarker Complete(const Marker marker, SyntaxKind kind) {
    const size_t position = marker.Position();

    Event<TokenKind, SyntaxKind>& event = events_.at(position);
    if (!std::holds_alternative<PlaceholderEvent>(event)) {
      throw std::invalid_argument(
          "cannot complete a marker that isn't a placeholder");
    }

    events_[position] = StartEvent(kind, std::nullopt);
    events_.emplace_back(FinishEvent{});

    return CompletedMarker(marker.Position());
  }

  void Expect(const TokenKind kind) noexcept {
    if (At(kind)) {
      Bump();
    } else {
      Error();
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

  // TODO(tamiyo) Implement parser support.
  void Error() {
    const std::optional<Token> token = PeekToken();

    const std::optional<TokenKind> found =
        token.value_or(std::nullopt).Kind<TokenKind>();

    const Span span = token.value_or(source_.LastTokenSpan().value()).Span();

    const ParseError error = ParseError{
        .expected = std::vector(expected_kinds_), .found = found, .span = span};

    events_.emplace_back(ErrorEvent(error));

    if (!AtSet(kRecoverySet)) {
      const Marker m = Start();
      Bump();
      Complete(m, SyntaxKind::kError);
    }
  }

 private:
  [[nodiscard]] bool AtSet(std::array<TokenKind, 0> recovery_set) {
    if (const auto kind = PeekKind(); kind.has_value()) {
      const TokenKind* found = std::find(std::begin(recovery_set),
                                         std::end(recovery_set), kind.value());
      return found != std::end(recovery_set);
    }

    return false;
  }

  [[nodiscard]] std::optional<Token> PeekToken() { return source_.PeekToken(); }

  [[nodiscard]] std::optional<TokenKind> PeekKind() {
    return source_.PeekKind();
  }

  constexpr std::array<TokenKind, 0> kRecoverySet = {};

  TokenSource source_;
  std::vector<Event<TokenKind, SyntaxKind>> events_;
  std::vector<TokenKind> expected_kinds_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_PARSER_H_
