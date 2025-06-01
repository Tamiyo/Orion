#ifndef LANG_PARSER_GRAMMAR_STATEMENT_H_
#define LANG_PARSER_GRAMMAR_STATEMENT_H_

#include <optional>

#include "lang/parser/parser.h"

namespace yuzu::lang {
syntax::CompletedMarker Root(Parser* p) noexcept;

std::optional<syntax::CompletedMarker> Stmt(Parser* p) noexcept;
}  // namespace yuzu::lang

#endif  // LANG_PARSER_GRAMMAR_STATEMENT_H_
