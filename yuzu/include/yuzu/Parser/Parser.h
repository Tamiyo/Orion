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

  /// Peek the kind of the nth-following non-trivia token. `offset` 0 (the
  /// default) is the next token; 1 is the one after it, and so on.
  [[nodiscard]] std::optional<lexer::TokenKind> peekKind(size_t offset = 0) {
    return source.peekKindAhead(offset);
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

  /// \brief Report a "this position should have been an expression" error.
  ///
  /// Same recovery shape as `error()` (snapshots the offending token,
  /// optionally bumps + wraps in an ERROR node), but captures a
  /// semantic expectation instead of the auto-collected `expectedKinds`
  /// list. The resulting diagnostic reads `expected expression, found
  /// `<text>`` (or `... end of input` at EOF), which is far less
  /// jargon-y than enumerating Number/Ident/LeftParen.
  ///
  /// Call from grammar rules that *know* they were trying to parse an
  /// expression — e.g. `parseLhs` when none of its branches matched.
  void errorExpression(
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
  TokenSource source;
  std::vector<Event> events;
  std::vector<lexer::TokenKind> expectedKinds;
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_PARSER_H
