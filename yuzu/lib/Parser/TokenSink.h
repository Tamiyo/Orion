#ifndef YUZU_PARSER_TOKEN_SINK_H
#define YUZU_PARSER_TOKEN_SINK_H

#include "yuzu/Ast/SyntaxKind.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Green/GreenBuilder.h"
#include "yuzu/Syntax/SyntaxKind.h"
#include "yuzu/Util/ErrorHandling.h"

#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace yuzu::parser {
class TokenSink final {
public:
  struct Result {
    const syntax::GreenNode green;
    const std::vector<std::string> errors;
  };

  explicit TokenSink(const std::vector<lexer::Token> &tokens,
                     const std::vector<Event> &events)
      : builder(syntax::GreenBuilder()), tokens(std::move(tokens)),
        events(std::move(events)), errors(std::vector<std::string>{}),
        cursor(0) {}

  Result finish() noexcept {
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
        addError(errorEvent->message);
      } else if (std::get_if<PlaceholderEvent>(&event)) {
        // Skip - already processed via forward parent
      } else {
        util::yuzu_unreachable();
      }

      bumpTrivia();
    }

    return Result{.green = builder.finish(), .errors = std::move(errors)};
  }

private:
  void startNode(const size_t startEventIdx, const ast::SyntaxKind startKind,
                 const std::optional<size_t> startForwardParent) noexcept {
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

  void finishNode() noexcept { builder.finishNode(); }

  void addToken() noexcept {
    const auto &token = tokens.at(cursor);
    builder.token(static_cast<syntax::SyntaxKind>(token.getKind()),
                  token.getSource());

    cursor += 1;
  }

  void addError(const std::string &error) noexcept {
    errors.emplace_back(error);
  }

  void bumpTrivia() noexcept {
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
  std::vector<std::string> errors;
  size_t cursor;
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_TOKEN_SINK_H
