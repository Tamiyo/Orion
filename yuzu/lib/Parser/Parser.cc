#include "yuzu/Parser/Parser.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/ParseError.h"

#include <bitset>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace yuzu::parser {

namespace {

/// Inspect what the parser is at: either the next pending token (the
/// normal "found X" case) or, at end-of-input, the just-consumed token
/// (so the error span at least falls on something the user typed).
struct FoundTokenInfo {
  std::optional<lexer::TokenKind> kind;
  std::u32string text; // empty when we truly fell off the end
  lexer::Range range;
};

FoundTokenInfo inspectFoundToken(TokenSource &source) {
  if (const std::optional<lexer::Token> nextToken = source.peekNextToken()) {
    return FoundTokenInfo{
        .kind = nextToken->getKind(),
        .text = std::u32string(nextToken->getSource()),
        .range = nextToken->getRange(),
    };
  }

  const std::optional<lexer::Token> lastToken = source.peekLastToken();
  if (!lastToken.has_value()) {
    // Truly empty source — no tokens at all.
    return FoundTokenInfo{
        .kind = std::nullopt,
        .text = std::u32string(),
        .range = lexer::Range{.start = 0, .end = 0},
    };
  }

  return FoundTokenInfo{
      .kind = std::nullopt,
      .text = std::u32string(),
      .range = lastToken->getRange(),
  };
}

} // namespace

void Parser::error(
    const std::bitset<1 << (8 * sizeof(lexer::TokenKind))> &recoverySet) {
  const FoundTokenInfo info = inspectFoundToken(source);

  // Snapshot the accumulated "we'd have accepted X" set; clear it so the
  // next decision starts fresh.
  std::vector<lexer::TokenKind> expected = std::move(expectedKinds);
  expectedKinds.clear();

  // The parser stays unaware of the diagnostics layer — push a structured
  // ParseError into the event stream and let the TokenSink translate it
  // into a Diagnostic at sink time.
  events.emplace_back(ErrorEvent{
      .error = std::make_unique<ExpectedKindError>(std::move(expected),
                                                   info.kind, info.range),
  });

  // If not at a recovery set and not at the end, inject an ERROR node into
  // the syntax tree marking this branch as corrupted. The diagnostic
  // explains the error; the Error node preserves the offending text in
  // the green tree.
  if (!atRecoverySet(recoverySet) && !atEnd()) {
    const Marker m = start();
    bump();
    auto _ = complete(m, ast::SyntaxKind::Error);
  }
}

void Parser::errorExpression(
    const std::bitset<1 << (8 * sizeof(lexer::TokenKind))> &recoverySet) {
  const FoundTokenInfo info = inspectFoundToken(source);

  // Drop the auto-collected expectedKinds — `errorExpression` carries a
  // semantic label, so the kinds-we-tried list isn't part of the
  // diagnostic. Still clear so the next decision starts fresh.
  expectedKinds.clear();

  events.emplace_back(ErrorEvent{
      .error = std::make_unique<ExpectedExpressionError>(std::move(info.text),
                                                         info.range),
  });

  if (!atRecoverySet(recoverySet) && !atEnd()) {
    const Marker m = start();
    bump();
    auto _ = complete(m, ast::SyntaxKind::Error);
  }
}

} // namespace yuzu::parser
