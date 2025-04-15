#ifndef LANG_PARSER_GRAMMAR_EXPRESSION_EXPRESSION_H_
#define LANG_PARSER_GRAMMAR_EXPRESSION_EXPRESSION_H_

#include <optional>

#include "lang/parser/parser.h"
#include "syntax/parser/marker.h"

namespace yuzu::lang {
std::optional<syntax::CompletedMarker> Expr(Parser* parser) noexcept;
}  // namespace yuzu::lang

#endif  // LANG_PARSER_GRAMMAR_EXPRESSION_EXPRESSION_H_
