#ifndef LANG_PARSER_PARSER_H_
#define LANG_PARSER_PARSER_H_

#include "lang/lexer/token_kind.h"
#include "lang/parser/syntax_kind.h"
#include "syntax/parser/parser.h"

namespace yuzu::lang {
using Parser = syntax::Parser<TokenKind, SyntaxKind>;
}  // namespace yuzu::lang

#endif  // LANG_PARSER_PARSER_H_
