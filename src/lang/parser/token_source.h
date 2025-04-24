#ifndef LANG_PARSER_TOKEN_SOURCE_H_
#define LANG_PARSER_TOKEN_SOURCE_H_

#include "lang/lexer/token_kind.h"
#include "syntax/parser/token_source.h"

namespace yuzu::lang {
using TokenSource = syntax::TokenSource<TokenKind>;
}  // namespace yuzu::lang

#endif  // LANG_PARSER_TOKEN_SOURCE_H_
