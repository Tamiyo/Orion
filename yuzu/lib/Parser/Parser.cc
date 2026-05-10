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
#include <utility>
#include <vector>

namespace yuzu::parser {

void Parser::error(
    const std::bitset<1 << (8 * sizeof(lexer::TokenKind))> &recoverySet) {
  // Identify what the parser is looking at: either the next pending token
  // (the normal "found X" case) or, at end-of-input, the just-consumed
  // token (so the error span at least falls on something the user typed).
  std::optional<lexer::TokenKind> found;
  std::optional<lexer::Range> range;
  if (const std::optional<lexer::Token> nextToken = source.peekNextToken()) {
    found = nextToken->getKind();
    range.emplace(nextToken->getRange());
  } else {
    const std::optional<lexer::Token> lastToken = source.peekLastToken();
    if (!lastToken.has_value()) {
      range.emplace(lexer::Range{.start = 0, .end = 0});
    } else {
      range.emplace(lastToken->getRange());
    }
    found = std::nullopt;
  }

  // Snapshot the accumulated "we'd have accepted X" set; clear it so the
  // next decision starts fresh.
  std::vector<lexer::TokenKind> expected = std::move(expectedKinds);
  expectedKinds.clear();

  // The parser stays unaware of the diagnostics layer — push a structured
  // ParseError into the event stream and let the TokenSink translate it
  // into a Diagnostic at sink time.
  events.emplace_back(ErrorEvent{
      .error = std::make_unique<ExpectedKindError>(std::move(expected), found,
                                                   range.value()),
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

} // namespace yuzu::parser
