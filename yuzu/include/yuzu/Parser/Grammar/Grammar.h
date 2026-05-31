#ifndef YUZU_PARSER_GRAMMAR_GRAMMAR_H
#define YUZU_PARSER_GRAMMAR_GRAMMAR_H

#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {
/// Parses a root.
std::optional<CompletedMarker> parseRoot(Parser &p);

/// Parses an identifier.
std::optional<CompletedMarker> parseIdent(Parser &p);
} // namespace yuzu::parser

#endif // YUZU_PARSER_GRAMMAR_GRAMMAR_H
