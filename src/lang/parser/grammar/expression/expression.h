#ifndef LANG_PARSER_GRAMMAR_EXPRESSION_EXPRESSION_H_
#define LANG_PARSER_GRAMMAR_EXPRESSION_EXPRESSION_H_

#include <optional>

#include "lang/parser/parser.h"
#include "syntax/parser/marker.h"

// https://matklad.github.io/2020/04/13/simple-but-powerful-pratt-parsing.html
namespace yuzu::lang {
std::optional<syntax::CompletedMarker> Expr(Parser* p) noexcept;
}  // namespace yuzu::lang

#endif  // LANG_PARSER_GRAMMAR_EXPRESSION_EXPRESSION_H_
