#include "lang/parser/grammar/stmt.h"

#include <optional>

#include "lang/lexer/token_kind.h"
#include "lang/parser/grammar/expr.h"
#include "lang/parser/parser.h"
#include "lang/parser/syntax_kind.h"

namespace yuzu::lang {
syntax::CompletedMarker Root(Parser* p) noexcept {
  const syntax::Marker m = p->Start();

  while (!p->At(TokenKind::kEof)) {
    Stmt(p);
  }

  p->Bump();  // Eat <eof>.

  return p->Complete(m, SyntaxKind::kRoot);
}

std::optional<syntax::CompletedMarker> Stmt(Parser* p) noexcept {
  // TODO(tamiyo) return more expressions
  return Expr(p);
}
}  // namespace yuzu::lang
