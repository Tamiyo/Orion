#ifndef YUZU_PARSER_GRAMMAR_STMT_H
#define YUZU_PARSER_GRAMMAR_STMT_H

#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {
std::optional<CompletedMarker> parseStmt(Parser &p);
} // namespace yuzu::parser

#endif // YUZU_PARSER_GRAMMAR_STMT_H
