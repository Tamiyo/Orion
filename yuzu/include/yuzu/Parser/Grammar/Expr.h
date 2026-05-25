#ifndef YUZU_PARSER_GRAMMAR_EXPR_H
#define YUZU_PARSER_GRAMMAR_EXPR_H

#include "yuzu/Parser/Marker.h"
#include "yuzu/Parser/Parser.h"

#include <optional>

namespace yuzu::parser {
std::optional<CompletedMarker>
parseExprBindingPower(Parser &p, const size_t minimumBindingPower);

std::optional<CompletedMarker> parseIdentExpr(Parser &p);

inline std::optional<CompletedMarker> parseExpr(Parser &p) {
  return parseExprBindingPower(p, 0);
}
} // namespace yuzu::parser

#endif // YUZU_PARSER_GRAMMAR_EXPR_H
