#ifndef LANG_LEXER_TOKEN_H_
#define LANG_LEXER_TOKEN_H_

#include "lang/lexer/token_kind.h"
#include "syntax/lexer/token.h"

namespace yuzu::lang {
using Token = syntax::Token<TokenKind>;
}  // namespace yuzu::lang

#endif  // LANG_LEXER_TOKEN_H_
