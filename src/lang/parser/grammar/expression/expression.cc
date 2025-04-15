#include "lang/parser/grammar/expression/expression.h"

#include <array>
#include <cstdint>
#include <optional>
#include <tuple>

#include "lang/lexer/token_kind.h"
#include "lang/parser/grammar/expression/infix_op.h"
#include "lang/parser/grammar/expression/postfix_op.h"
#include "lang/parser/grammar/expression/prefix_op.h"
#include "lang/parser/parser.h"
#include "syntax/parser/marker.h"

namespace yuzu::lang {
namespace {
constexpr std::array<TokenKind, 0> kExprRecoverySet = {};

std::optional<syntax::CompletedMarker> Lhs(Parser* parser) noexcept {
  if (parser->At(TokenKind::kIdentifier)) {
  } else {
    parser->Error(kExprRecoverySet);
    return std::nullopt;
  }

  // TODO(tamiyo) All branches should return a valid completed marker.
  return std::nullopt;
}

std::optional<syntax::CompletedMarker> ExprBindingPower(
    Parser* parser, uint8_t minimum_binding_power) noexcept {
  std::optional<syntax::CompletedMarker> lhs = Lhs(parser);
  if (!lhs) {
    return std::nullopt;
  }

  while (true) {
    // TODO(tamiyo) Replace with TokenKind to InfixOp conversion.
    // TODO(tamiyo) Should this be called infix or binary?
    const auto op = InfixOp::kAdd;

    const auto [left_binding_power, right_bnding_power] = BindingPower(op);

    if (left_binding_power < minimum_binding_power) {
      break;
    }

    // Eat the operator's token.
    parser->Bump();

    auto marker = parser->Precede(*lhs);
    auto parsed_rhs = ExprBindingPower(parser, right_bnding_power).has_value();
    lhs = parser->Complete(marker, SyntaxKind::kInfixExpr);

    if (!parsed_rhs) {
      break;
    }
  }

  return lhs;
}
};  // namespace

std::optional<syntax::CompletedMarker> Expr(Parser* parser) noexcept {
  return ExprBindingPower(parser, 0);
}
}  // namespace yuzu::lang
