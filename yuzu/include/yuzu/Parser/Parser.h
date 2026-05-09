#ifndef YUZU_PARSER_PARSER_H
#define YUZU_PARSER_PARSER_H

#include "yuzu/Ast/Ast.h"
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
class [[nodiscard]] Parser final {
public:
  explicit Parser(const TokenSource &source) : source(std::move(source)) {}

  Parser() = delete;

  Marker start() {
    const size_t position = events.size();
    events.emplace_back(PlaceholderEvent{});
    return Marker{.position = position};
  }

  CompletedMarker complete(const Marker &marker, ast::SyntaxKind kind) {
    Event &eventAtPosition = events[marker.position];
    assert(std::holds_alternative<PlaceholderEvent>(eventAtPosition) &&
           "Cannot complete a marker that points to non-placeholder events.");

    const auto _ = eventAtPosition.exchange(
        StartEvent{.forwardParent = std::nullopt, .kind = kind});

    events.emplace_back(FinishEvent{});

    return CompletedMarker{.position = marker.position};
  }

  [[nodiscard("Preceded markers should not be discarded.")]]
  std::pair<Marker, ast::SyntaxKind>
  precede(const CompletedMarker &completedMarker) {
    const Marker newMarker = start();

    Event &eventAtPosition = events[completedMarker.position];
    if (const StartEvent *startEvent =
            std::get_if<StartEvent>(&eventAtPosition)) {

      const ast::SyntaxKind newKind = startEvent->kind;
      const size_t newForwardParent =
          newMarker.position - completedMarker.position;

      const auto _ = eventAtPosition.exchange(
          StartEvent{.forwardParent = newForwardParent, .kind = newKind});

      return std::make_pair(newMarker, newKind);
    }

    util::yuzu_unreachable();
  }

  [[nodiscard]] std::optional<lexer::TokenKind> peekKind() {
    return source.peekNextKind();
  }

  [[nodiscard]] bool at(lexer::TokenKind kind) {
    expectedKinds.emplace_back(kind);
    return peekKind() == kind;
  }

  [[nodiscard]] bool atEnd() { return !peekKind().has_value(); }

  [[nodiscard]] bool atRecoverySet(
      const std::bitset<1 << (8 * sizeof(lexer::TokenKind))> &recoverySet) {
    if (const std::optional<lexer::TokenKind> kind = peekKind()) {
      const size_t pos = static_cast<size_t>(kind.value());
      return recoverySet.test(pos);
    }

    return false;
  }

  void error(
      const std::bitset<1 << (8 * sizeof(lexer::TokenKind))> &recoverySet = {});

  void bump() {
    expectedKinds.clear();
    const std::optional<lexer::Token> _ = source.getNextToken();
    events.emplace_back(TokenEvent{});
  }

  void expect(lexer::TokenKind kind) {
    if (at(kind)) {
      bump();
    } else {
      error();
    }
  }

  /// Consume the parser and return its accumulated event stream so it can be
  /// fed to a TokenSink. Intended for end-of-parse use only.
  [[nodiscard]] std::vector<Event> finish() && { return std::move(events); }

private:
  std::vector<Event> events;
  std::vector<lexer::TokenKind> expectedKinds;
  TokenSource source;
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_PARSER_H
