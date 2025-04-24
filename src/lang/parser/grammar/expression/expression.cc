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
#include "lang/parser/syntax_kind.h"
#include "syntax/parser/marker.h"

namespace yuzu::lang {
constexpr std::array<TokenKind, 0> kExprRecoverySet = {};

std::optional<syntax::CompletedMarker> ExprBindingPower(
    Parser* p, uint8_t minimum_binding_power) noexcept;

std::optional<syntax::CompletedMarker> Lhs(Parser* p) noexcept {
  const std::optional<TokenKind> kind = p->PeekKind();
  if (!kind) {
    return std::nullopt;
  }

  switch (kind.value()) {
    case TokenKind::kUnquotedIdent:
    case TokenKind::kQuotedIdent: {
      const syntax::Marker m = p->Start();
      p->Bump();
      return p->Complete(m, SyntaxKind::kIdent);
    }

    case TokenKind::kBooleanLiteral:
    case TokenKind::kStringLiteral:
    case TokenKind::kBigDecimalLiteral:
    case TokenKind::kBigIntLiteral:
    case TokenKind::kIntLiteral:
    case TokenKind::kSmallIntLiteral:
    case TokenKind::kTinyIntLiteral:
    case TokenKind::kFloatLiteral:
    case TokenKind::kDoubleLit: {
      const syntax::Marker m = p->Start();
      p->Bump();
      return p->Complete(m, SyntaxKind::kLiteral);
    }

    case TokenKind::kPlus:
    case TokenKind::kMinus: {
      const syntax::Marker m = p->Start();
      const auto [_, right_binding_power] =
          PrefixBindingPower(p->PeekKind()).value();

      p->Bump();  // Eat 'op'

      ExprBindingPower(p, right_binding_power);
      return p->Complete(m, SyntaxKind::kPrefixExpr);
    }

    case TokenKind::kLeftParen: {
      const syntax::Marker m = p->Start();
      p->Bump();  // eat '('
      ExprBindingPower(p, 0);
      p->Expect(TokenKind::kRightParen);
      return p->Complete(m, SyntaxKind::kParenExpr);
    }

    default: {
      p->Error(kExprRecoverySet);
      return std::nullopt;
    }
  }
}

std::optional<syntax::CompletedMarker> ExprBindingPower(
    Parser* p, const uint8_t minimum_binding_power) noexcept {
  std::optional<syntax::CompletedMarker> lhs = Lhs(p);
  if (!lhs) {
    return std::nullopt;
  }

  while (true) {
    // Postfix Operators
    if (const std::optional<std::tuple<uint8_t, uint8_t>> bp =
            PostfixBindingPower(p->PeekKind());
        bp.has_value()) {
      const auto [left_binding_power, _] = bp.value();
      if (left_binding_power < minimum_binding_power) {
        break;
      }

      p->Bump();  // Eat the prefix operators's token.

      const std::optional<TokenKind> kind = p->PeekKind();
      if (!kind) {
        break;
      }

      switch (kind.value()) {
        case TokenKind::kLeftSquare: {
          const syntax::Marker m = p->Start();
          p->Bump();  // Eat '['.
          ExprBindingPower(p, 0);
          p->Expect(TokenKind::kRightSquare);
          return p->Complete(m, SyntaxKind::kIndex);
        }

        default: {
          // unreachable
        }
      }

      continue;
    }

    // Infix Operators
    if (const std::optional<std::tuple<uint8_t, uint8_t>> bp =
            InfixBindingPower(p->PeekKind());
        bp.has_value()) {
      const auto [left_binding_power, right_binding_power] = bp.value();
      if (left_binding_power < minimum_binding_power) {
        break;
      }

      p->Bump();  // Eat the infix operator's token.

      const syntax::Marker m = p->Precede(*lhs);
      const std::optional<syntax::CompletedMarker> rhs =
          ExprBindingPower(p, right_binding_power);

      lhs = p->Complete(m, SyntaxKind::kInfixExpr);

      if (!rhs.has_value()) {
        break;
      }

      continue;
    }

    break;
  }

  return lhs;
}

std::optional<syntax::CompletedMarker> Expr(Parser* p) noexcept {
  return ExprBindingPower(p, 0);
}
}  // namespace yuzu::lang
