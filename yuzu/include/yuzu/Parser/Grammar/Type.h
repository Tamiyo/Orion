#ifndef YUZU_PARSER_GRAMMAR_TYPE_H
#define YUZU_PARSER_GRAMMAR_TYPE_H

#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {
/// Parse a type expression — the syntactic form of a type as written in
/// source. Produces one of `NamedType` (`string`, `Relation[Employee]`),
/// `FuncType` (`(int, str) -> bool`), or `RecordType`
/// (`(sum: decimal, count: int64)`).
std::optional<CompletedMarker> parseType(Parser &p);
} // namespace yuzu::parser

#endif // YUZU_PARSER_GRAMMAR_TYPE_H
