#ifndef SYNTAX_PARSER_TOKEN_SINK_H_
#define SYNTAX_PARSER_TOKEN_SINK_H_

#include <utility>
#include <variant>
#include <vector>

#include "syntax/lexer/token.h"
#include "syntax/parser/event/event.h"
#include "syntax/parser/rgtree/green/green_builder.h"
#include "syntax/syntax_kind.h"

namespace orion::syntax {
class TokenSink {
 public:
  explicit TokenSink(std::vector<Token> tokens, std::vector<Event> events)
      : tokens_(std::move(tokens)),
        token_idx_(0),
        text_idx_(0),
        events_(std::move(events)),
        builder_(GreenBuilder()) {}

  TokenSink() = delete;

  void Finish();

 private:
  void StartNode(size_t event_idx, SyntaxKind kind,
                 std::optional<size_t> forward_parent);

  void FinishNode() noexcept { builder_.FinishNode(); }

  void AddToken() noexcept {
    const Token token = tokens_.at(token_idx_);
    builder_.Token(token.Kind<SyntaxKind>(), token.Source());

    token_idx_ += 1;
    text_idx_ += token.Length();
  }

  const std::vector<Token> tokens_;
  size_t token_idx_;
  size_t text_idx_;
  std::vector<Event> events_;
  GreenBuilder builder_;
};
}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_TOKEN_SINK_H_
