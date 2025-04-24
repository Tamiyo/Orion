#ifndef LANG_PARSER_TOKEN_SINK_H_
#define LANG_PARSER_TOKEN_SINK_H_

#include "lang/lexer/token_kind.h"
#include "lang/parser/syntax_kind.h"
#include "syntax/parser/token_sink.h"

namespace yuzu::lang {
using TokenSink = syntax::TokenSink<TokenKind, SyntaxKind>;
}  // namespace yuzu::lang

#endif  // LANG_PARSER_TOKEN_SINK_H_
