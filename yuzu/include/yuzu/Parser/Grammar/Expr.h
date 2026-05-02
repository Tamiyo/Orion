#ifndef YUZU_PARSER_GRAMMAR_EXPR_H
#define YUZU_PARSER_GRAMMAR_EXPR_H

#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {
std::optional<CompletedMarker>
parseExprBindingPower(Parser &parser, const size_t minimumBindingPower);

inline std::optional<CompletedMarker> parseExpr(Parser &parser) {
  return parseExprBindingPower(parser, 0);
}
} // namespace yuzu::parser

#endif // YUZU_PARSER_GRAMMAR_EXPR_H
