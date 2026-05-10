#include "yuzu/Parser/Parser.h"

#include "yuzu/Ast/Ast.h"
#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/Token.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Parser/Event.h"
#include "yuzu/Parser/ParseError.h"

#include <algorithm>
#include <bitset>
#include <iterator>
#include <memory>
#include <optional>
#include <vector>

namespace yuzu::parser {
void Parser::error(
    const std::bitset<1 << (8 * sizeof(lexer::TokenKind))> &recoverySet) {
  std::optional<lexer::TokenKind> found;
  std::optional<lexer::Range> range;

  // Try to figure out the ranges of the next (or last) token to form an error
  // message.
  if (const std::optional<lexer::Token> nextToken = source.peekNextToken()) {
    found = nextToken->getKind();
    range.emplace(nextToken->getRange());
  } else {
    const std::optional<lexer::Token> lastToken = source.peekLastToken();
    if (!lastToken.has_value()) {
      range.emplace(lexer::Range{.start = 0, .end = 0});
    }

    found = std::nullopt;
    range.emplace(lastToken->getRange());
  }

  // Copy all expected kinds into a new vector, and clear the previous one.
  std::vector<lexer::TokenKind> expected;
  std::move(expectedKinds.begin(), expectedKinds.end(),
            std::back_inserter(expected));

  expectedKinds.clear();

  events.emplace_back(
      ErrorEvent{.error = std::make_unique<ExpectedKindError>(
                     std::move(expected), found, range.value())});

  // If not at a recovery set and not at the end, inject an ERROR node into
  // the syntax tree marking this branch as corrupted.
  if (!atRecoverySet(recoverySet) && !atEnd()) {
    const Marker m = start();
    bump();
    auto _ = complete(m, ast::SyntaxKind::Error);
  }
}

} // namespace yuzu::parser
