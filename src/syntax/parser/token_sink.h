#ifndef SYNTAX_PARSER_TOKEN_SINK_H_
#define SYNTAX_PARSER_TOKEN_SINK_H_

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "syntax/lexer/token.h"
#include "syntax/parser/error/parse_error.h"
#include "syntax/parser/event.h"
#include "syntax/parser/rgtree/green/green.h"
#include "syntax/parser/rgtree/green/green_builder.h"

namespace yuzu::syntax {
/// \brief TokenSink is responsible for consuming a sequence of parser events
/// and tokens, and producing a syntax tree via the GreenBuilder.
/// It processes events such as starting/finishing nodes, adding tokens,
/// and handles structure assembly of the parsed source.
template <typename TokenKind = uint16_t, typename SyntaxKind = uint16_t>
class TokenSink {
 private:
  using ErrorEvent = ErrorEvent<TokenKind>;
  using Event = Event<TokenKind, SyntaxKind>;
  using GreenBuilder = GreenBuilder<SyntaxKind>;
  using GreenNode = GreenNode<SyntaxKind>;
  using ParseError = ParseError<TokenKind>;
  using StartEvent = StartEvent<SyntaxKind>;
  using Token = Token<TokenKind>;

 public:
  struct Result {
    const GreenNode node;
    const std::vector<ParseError> errors;
  };

 public:
  /// \brief Constructs a TokenSink with a stream of tokens and events.
  /// \param tokens The input token stream from the lexer.
  /// \param events The list of parser events describing how to build the syntax
  /// \param is_trivia Determines if a TokenKind is a trivia token.
  /// tree.
  explicit TokenSink(const std::vector<Token>& tokens,
                     const std::vector<Event>& events,
                     const std::function<bool(TokenKind)>& is_trivia)
      : tokens_(std::move(tokens)),
        is_trivia_(is_trivia),
        token_idx_(0),
        text_idx_(0),
        events_(std::move(events)),
        errors_({}),
        builder_(GreenBuilder()) {}

  // Deleted default constructor to enforce required token/event input.
  TokenSink() = delete;

  /// \brief Bumps the event stream and builds the corresponding green tree.
  /// This is the main entry point for transforming parser output into
  /// an immutable syntax tree structure.
  Result Finish() {
    for (size_t event_idx = 0; event_idx < events_.size(); event_idx++) {
      if (const Event& event = events_[event_idx];
          std::holds_alternative<StartEvent>(event)) {
        const auto e = std::get<StartEvent>(event);
        events_[event_idx] = PlaceholderEvent{};

        StartNode(event_idx, e.Kind(), e.ForwardParent());
      } else if (std::holds_alternative<FinishEvent>(event)) {
        FinishNode();
      } else if (std::holds_alternative<TokenEvent>(event)) {
        AddToken();
      } else if (std::holds_alternative<ErrorEvent>(event)) {
        const auto e = std::get<ErrorEvent>(event);
        errors_.emplace_back(e.Error());
      } else if (std::holds_alternative<PlaceholderEvent>(event)) {
        // Do Nothing
      } else {
        throw std::invalid_argument("unknown event type");
      }

      BumpTrivia();
    }

    return Result{.node = builder_.Finish(), .errors = std::move(errors_)};
  }

 private:
  /// \brief Handles the Start event by beginning a new syntax node.
  /// \param event_idx The index of the event being processed.
  /// \param kind The kind of the syntax node.
  /// \param forward_parent Optional index to the forward parent for delayed
  /// nesting.
  void StartNode(const size_t event_idx, SyntaxKind kind,
                 const std::optional<size_t> forward_parent) {
    size_t event_idx_mut = event_idx;
    std::optional<size_t> forward_parent_mut = forward_parent;

    std::vector<SyntaxKind> kinds = {kind};
    while (forward_parent_mut.has_value()) {
      event_idx_mut += forward_parent_mut.value();

      if (const Event& event = events_[event_idx_mut];
          std::holds_alternative<StartEvent>(event)) {
        const auto e = std::get<StartEvent>(event);
        events_[event_idx_mut] = PlaceholderEvent{};

        kinds.emplace_back(e.Kind());
        forward_parent_mut = e.ForwardParent();
      } else {
        throw std::invalid_argument("unreachable event in StartNode");
      }
    }

    for (auto& k : std::ranges::reverse_view(kinds)) {
      builder_.StartNode(k);
    }
  }

  /// \brief Handles the Finish event by closing the current syntax node.
  void FinishNode() noexcept { builder_.FinishNode(); }

  /// \brief Handles the Token event by appending the next token to the syntax
  /// tree.
  void AddToken() noexcept {
    const Token& token = tokens_.at(token_idx_);
    builder_.Token(static_cast<SyntaxKind>(token.Kind()), token.Source());

    token_idx_ += 1;
    text_idx_ += token.Length();
  }

  void BumpTrivia() {
    while (token_idx_ < tokens_.size()) {
      if (const Token& token = tokens_.at(token_idx_);
          !is_trivia_(token.Kind())) {
        break;
      }

      AddToken();
    }
  }

  const std::vector<Token> tokens_;
  const std::function<bool(TokenKind)>& is_trivia_;
  size_t token_idx_;
  size_t text_idx_;
  std::vector<Event> events_;
  std::vector<ParseError> errors_;
  GreenBuilder builder_;
};
}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_TOKEN_SINK_H_
