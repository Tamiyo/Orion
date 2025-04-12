#ifndef SYNTAX_PARSER_TOKEN_SINK_H_
#define SYNTAX_PARSER_TOKEN_SINK_H_

#include <utility>
#include <variant>
#include <vector>

#include "syntax/lexer/token.h"
#include "syntax/parser/event.h"
#include "syntax/parser/rgtree/green/green_builder.h"
#include "syntax/parser/rgtree/green/green_node.h"
#include "syntax/syntax_kind.h"

namespace orion::syntax {
/// \brief TokenSink is responsible for consuming a sequence of parser events
/// and tokens, and producing a syntax tree via the GreenBuilder.
/// It processes events such as starting/finishing nodes, adding tokens,
/// and handles structure assembly of the parsed source.
class TokenSink {
 public:
  struct Result {
    const GreenNode node;
  };

 public:
  /// \brief Constructs a TokenSink with a stream of tokens and events.
  /// \param tokens The input token stream from the lexer.
  /// \param events The list of parser events describing how to build the syntax
  /// tree.
  explicit TokenSink(std::vector<Token> tokens, std::vector<Event> events)
      : tokens_(std::move(tokens)),
        token_idx_(0),
        text_idx_(0),
        events_(std::move(events)),
        builder_(GreenBuilder()) {}

  // Deleted default constructor to enforce required token/event input.
  TokenSink() = delete;

  /// \brief Bumps the event stream and builds the corresponding green tree.
  /// This is the main entry point for transforming parser output into
  /// an immutable syntax tree structure.
  Result Finish();

 private:
  /// \brief Handles the Start event by beginning a new syntax node.
  /// \param event_idx The index of the event being processed.
  /// \param kind The kind of the syntax node.
  /// \param forward_parent Optional index to the forward parent for delayed
  /// nesting.
  void StartNode(size_t event_idx, SyntaxKind kind,
                 std::optional<size_t> forward_parent);

  /// \brief Handles the Finish event by closing the current syntax node.
  void FinishNode() noexcept { builder_.FinishNode(); }

  /// \brief Handles the Token event by appending the next token to the syntax
  /// tree.
  void AddToken() noexcept {
    const Token token = tokens_.at(token_idx_);
    builder_.Token(token.Kind<SyntaxKind>(), token.Source());

    token_idx_ += 1;
    text_idx_ += token.Length();
  }

  // Lexer-produced tokens used to build the final tree.
  const std::vector<Token> tokens_;

  // Current index in the tokens_ vector.
  size_t token_idx_;

  // Current byte offset into the input text.
  size_t text_idx_;

  // Sequence of parser events used to construct the syntax tree.
  std::vector<Event> events_;

  // The builder that creates the green (immutable) syntax tree nodes.
  GreenBuilder builder_;
};
}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_TOKEN_SINK_H_
