#ifndef SYNTAX_PARSER_PARSER_BASE_H_
#define SYNTAX_PARSER_PARSER_BASE_H_

#include <vector>

#include "syntax/lexer/token_kind.h"
#include "syntax/parser/event.h"
#include "syntax/parser/marker.h"
#include "syntax/parser/token_source.h"

namespace orion::syntax {
class ParserBase {
 public:
  ParserBase() = delete;
  virtual ~ParserBase() = default;

 protected:
  void Expect(const TokenKind kind) noexcept {
    if (At(kind)) {
      Bump();
    } else {
      Error();
    }
  }

  void Bump() noexcept {
    expected_kinds_.clear();
    auto _ = source_.NextToken();
    events_.emplace_back(Event::CreateToken());
  }

  void BumpMaybe(TokenKind kind) noexcept {
    if (At(kind)) {
      Bump();
    }
  }

  [[nodiscard]] bool At(const TokenKind kind) noexcept { return PeekKind() == kind; }

  [[nodiscard]] bool AtEnd() noexcept {
    return source_.PeekToken() == std::nullopt;
  }

  // TODO(tamiyo) Implement parser support.
  void Error() {}

 private:
  [[nodiscard]] std::optional<TokenKind> PeekKind() {
    return source_.PeekKind();
  }

  TokenSource source_;
  std::vector<Event> events_;
  std::vector<TokenKind> expected_kinds_;
};
}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_PARSER_BASE_H_
