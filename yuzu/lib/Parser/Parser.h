#ifndef YUZU_PARSER_PARSER_H
#define YUZU_PARSER_PARSER_H

#include "yuzu/Ast/SyntaxKind.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/TokenSource.h"
#include "yuzu/Util/ErrorHandling.h"

#include <bitset>
#include <cassert>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace yuzu::parser {
class Parser final {
public:
  explicit Parser(const TokenSource &source) : source(std::move(source)) {}

  Parser() = delete;

  [[nodiscard("Markers should not be discarded.")]] Marker start() noexcept {
    const size_t position = events.size();
    events.emplace_back(PlaceholderEvent{});
    return Marker{.position = position};
  }

  CompletedMarker complete(const Marker &marker,
                           ast::SyntaxKind kind) noexcept {
    Event& eventAtPosition = events[marker.position];
    assert(std::holds_alternative<PlaceholderEvent>(eventAtPosition) &&
           "Cannot complete a marker that points to non-placeholder events.");

    eventAtPosition.exchange(
        StartEvent{.forwardParent = std::nullopt, .kind = kind});

    events.emplace_back(FinishEvent{});

    return CompletedMarker{.position = marker.position};
  }

  [[nodiscard("Preceded markers should not be discarded.")]] std::pair<
      Marker, ast::SyntaxKind>
  precede(const CompletedMarker &complatedMarker) noexcept {
    const Marker newMarker = start();

    Event& eventAtPosition = events[complatedMarker.position];
    if (const StartEvent *startEvent =
            std::get_if<StartEvent>(&eventAtPosition)) {

      const ast::SyntaxKind newKind = startEvent->kind;
      const size_t newForwardParent =
          newMarker.position - complatedMarker.position;

      eventAtPosition.exchange(
          StartEvent{.forwardParent = newForwardParent, .kind = newKind});

      return std::make_pair(newMarker, newKind);
    }

    util::yuzu_unreachable();
  }

  [[nodiscard]] std::optional<lexer::TokenKind> peekKind() noexcept {
    return source.peekNextKind();
  }

  [[nodiscard]] bool at(lexer::TokenKind kind) noexcept {
    expectedKinds.emplace_back(kind);
    return peekKind() == kind;
  }

  [[nodiscard]] bool atEnd() noexcept { return !peekKind().has_value(); }

  [[nodiscard]] bool atRecoverySet(
      const std::bitset<sizeof(const lexer::TokenKind)> &recoverySet) noexcept {
    if (const std::optional<lexer::TokenKind> kind = peekKind()) {
      const size_t pos = static_cast<size_t>(kind.value());
      return recoverySet.test(pos);
    }

    return false;
  }

  void error(
      const std::bitset<sizeof(lexer::TokenKind)> &recoverySet = {}) noexcept;

  void bump() noexcept {
    expectedKinds.clear();
    const std::optional<lexer::Token> _ = source.getNextToken();
    events.emplace_back(TokenEvent{});
  }

  void expect(lexer::TokenKind kind) noexcept {
    if (at(kind)) {
      bump();
    } else {
      error();
    }
  }

private:
  std::vector<Event> events;
  std::vector<lexer::TokenKind> expectedKinds;
  TokenSource source;
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_PARSER_H