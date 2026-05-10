#ifndef YUZU_PARSER_TOKEN_SINK_H
#define YUZU_PARSER_TOKEN_SINK_H

#include "yuzu/Ast/Ast.h"
#include "yuzu/Diagnostics/DiagnosticsEngine.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Green/GreenBuilder.h"
#include "yuzu/Syntax/SyntaxKind.h"
#include "yuzu/Util/ErrorHandling.h"

#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace yuzu::parser {
/// \brief Consumes a parser's event stream into a green tree and emits
/// any structured `ErrorEvent`s as diagnostics.
///
/// The parser is intentionally unaware of the diagnostics layer — it
/// pushes structured `ErrorEvent`s describing what went wrong, and the
/// sink translates those into `Diagnostic`s on the engine. The split
/// keeps the parser focused on syntactic decisions and lets the sink
/// own the source-position-resolution work (since it already walks
/// the event stream).
class [[nodiscard]] TokenSink final {
public:
  struct Result {
    syntax::GreenNode green;
  };

  /// \brief Construct a sink bound to a tokens vector, an event stream,
  /// a diagnostics engine, and the `SourceId` the tokens came from.
  ///
  /// `engine` and `sourceId` are used together to translate `ErrorEvent`s
  /// into diagnostics with source-tagged spans. The engine and source map
  /// are owned by the caller; the sink just borrows.
  explicit TokenSink(std::vector<lexer::Token> tokens,
                     std::vector<Event> events,
                     diagnostics::DiagnosticsEngine &engine,
                     diagnostics::SourceId sourceId)
      : builder(syntax::GreenBuilder()), tokens(std::move(tokens)),
        events(std::move(events)), engine(engine), sourceId(sourceId),
        cursor(0) {}

  Result finish() {
    for (size_t eventIdx = 0, size = events.size(); eventIdx < size;
         eventIdx++) {
      const auto event = events[eventIdx].exchange(PlaceholderEvent{});

      if (const auto *startEvent = std::get_if<StartEvent>(&event)) {
        startNode(eventIdx, startEvent->kind, startEvent->forwardParent);
      } else if (std::get_if<FinishEvent>(&event)) {
        finishNode();
      } else if (std::get_if<TokenEvent>(&event)) {
        addToken();
      } else if (const auto *errorEvent = std::get_if<ErrorEvent>(&event)) {
        // The ParseError owns the message/label shape — the sink just
        // tags it with the source id and pushes onto the engine.
        engine.push(errorEvent->error->toDiagnostic(sourceId));
      } else if (std::get_if<PlaceholderEvent>(&event)) {
        // Skip - already processed via forward parent
      } else {
        util::yuzu_unreachable();
      }

      bumpTrivia();
    }

    return Result{.green = builder.finish()};
  }

private:
  void startNode(const size_t startEventIdx, const ast::SyntaxKind startKind,
                 const std::optional<size_t> startForwardParent) {
    size_t eventIdx = startEventIdx;
    std::optional<size_t> forwardParent = startForwardParent;

    auto kinds = std::vector<ast::SyntaxKind>{startKind};
    while (forwardParent.has_value()) {
      eventIdx += forwardParent.value();

      const auto event = events[eventIdx].exchange(PlaceholderEvent{});
      if (const auto *startEvent = std::get_if<StartEvent>(&event)) {
        kinds.emplace_back(startEvent->kind);
        forwardParent = startEvent->forwardParent;
      } else {
        util::yuzu_unreachable();
      }
    }

    for (auto it = kinds.rbegin(), end = kinds.rend(); it != end; ++it) {
      builder.startNode(static_cast<syntax::SyntaxKind>(*it));
    }
  }

  void finishNode() { builder.finishNode(); }

  void addToken() {
    const auto &token = tokens.at(cursor);
    // The lexer's `TokenKind::Error` lives just past `TOKENS_LAST` and so
    // doesn't share its numeric value with any `ast::SyntaxKind` token —
    // remap it explicitly to `ast::SyntaxKind::Error` so the green tree
    // stores a meaningful kind. Real tokens go through the value-preserving
    // cast, which works because the token blocks of `TokenKind` and
    // `ast::SyntaxKind` are kept numerically aligned.
    const syntax::SyntaxKind kind =
        token.getKind() == lexer::TokenKind::Error
            ? static_cast<syntax::SyntaxKind>(ast::SyntaxKind::Error)
            : static_cast<syntax::SyntaxKind>(token.getKind());

    builder.token(kind, token.getSource());
    cursor += 1;
  }

  void bumpTrivia() {
    while (cursor < tokens.size()) {
      if (!lexer::isTrivia(tokens.at(cursor).getKind())) {
        break;
      }

      addToken();
    }
  }

  syntax::GreenBuilder builder;
  const std::vector<lexer::Token> tokens;
  std::vector<Event> events;
  diagnostics::DiagnosticsEngine &engine;
  diagnostics::SourceId sourceId;
  size_t cursor;
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_TOKEN_SINK_H
