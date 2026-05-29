#include "yuzu/Ast/Ast.h"

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>

namespace yuzu::ast {
namespace {
static const std::u32string boolTrueValue = U"true";
static const std::u32string boolFalseValue = U"false";

/// Parse `text` (skipping the first `skip` chars for a base prefix like
/// `0x`/`0b`) as an integer in `base`. Underscores are stripped; any
/// other non-digit character or out-of-base digit returns nullopt.
std::optional<int64_t> parseIntegerLiteral(std::u32string_view text, int base,
                                           std::size_t skip) {
  int64_t value = 0;
  for (std::size_t i = skip; i < text.size(); ++i) {
    const char32_t c = text[i];
    if (c == U'_') {
      continue;
    }
    int digit;
    if (c >= U'0' && c <= U'9') {
      digit = static_cast<int>(c - U'0');
    } else if (c >= U'a' && c <= U'f') {
      digit = static_cast<int>(c - U'a') + 10;
    } else if (c >= U'A' && c <= U'F') {
      digit = static_cast<int>(c - U'A') + 10;
    } else {
      return std::nullopt;
    }
    if (digit >= base) {
      return std::nullopt;
    }
    value = value * base + digit;
  }
  return value;
}
} // namespace

std::optional<BinOp> BinaryExpr::getOp() const {
  for (const auto &child : node.getChildrenWithTokens()) {
    if (child.isToken()) {
      const auto &token = child.getToken();
      switch (token.getKind()) {
      case SyntaxKind::Minus:
        return BinOp::Sub;
      case SyntaxKind::Plus:
        return BinOp::Add;
      case SyntaxKind::Slash:
        return BinOp::Div;
      case SyntaxKind::Star:
        return BinOp::Mul;
      case SyntaxKind::Lt:
        return BinOp::Lt;
      case SyntaxKind::Lte:
        return BinOp::Lte;
      case SyntaxKind::Gt:
        return BinOp::Gt;
      case SyntaxKind::Gte:
        return BinOp::Gte;
      case SyntaxKind::ShiftLeft:
        return BinOp::ShiftLeft;
      case SyntaxKind::ShiftRight:
        return BinOp::ShiftRight;
      case SyntaxKind::InKw:
        return BinOp::In;
      case SyntaxKind::NotKw:
        // The only binary use of `not` is `not in`; the `not` token leads,
        // followed by the `in` token.
        return BinOp::NotIn;
      default:
        break;
      }

      break;
    }
  }

  return std::nullopt;
}

std::optional<UnaryOp> UnaryExpr::getOp() const {
  for (const auto &child : node.getChildrenWithTokens()) {
    if (child.isToken()) {
      const auto &token = child.getToken();
      switch (token.getKind()) {
      case SyntaxKind::Plus:
        return UnaryOp::Pos;
      case SyntaxKind::Minus:
        return UnaryOp::Neg;
      case SyntaxKind::NotKw:
        return UnaryOp::Not;
      default:
        break;
      }

      break;
    }
  }

  return std::nullopt;
}

std::optional<Bool> BoolLit::getValue() const {
  for (const auto &child : node.getChildrenWithTokens()) {
    if (!child.isToken()) {
      continue;
    }

    const auto &token = child.getToken();
    const auto text = token.getGreen().getSource();
    if (token.getKind() == SyntaxKind::BooleanLiteral) {
      if (text == boolTrueValue) {
        return true;
      }

      if (text == boolFalseValue) {
        return false;
      }
    }
    break;
  }

  return std::nullopt;
}

std::optional<Int64> IntLit::getValue() const {
  // Walk children-with-tokens; the first literal-shaped token wins.
  for (const auto &child : node.getChildrenWithTokens()) {
    if (!child.isToken()) {
      continue;
    }
    const auto &token = child.getToken();
    const auto text = token.getGreen().getSource();
    switch (token.getKind()) {
    case SyntaxKind::IntegerLiteral:
      return parseIntegerLiteral(text, /*base=*/10, /*skip=*/0);
    case SyntaxKind::HexLiteral:
      return parseIntegerLiteral(text, /*base=*/16, /*skip=*/2);
    case SyntaxKind::BinaryLiteral:
      return parseIntegerLiteral(text, /*base=*/2, /*skip=*/2);
    default:
      break;
    }
  }
  return std::nullopt;
}

std::optional<Float64> FloatLit::getValue() const {
  const auto floatToken = token(node, SyntaxKind::FloatLiteral);
  if (!floatToken) {
    return std::nullopt;
  }

  // Strip underscores into an ASCII buffer. Valid float text is all
  // ASCII (digits, `.`, `e`/`E`, `+`/`-`), so a `char` buffer is safe
  // and `std::from_chars` can parse it without locale or exceptions.
  std::string buffer;
  buffer.reserve(floatToken->getGreen().getSource().size());
  for (const char32_t c : floatToken->getGreen().getSource()) {
    if (c == U'_') {
      continue;
    }
    if (c > 127) {
      return std::nullopt;
    }
    buffer.push_back(static_cast<char>(c));
  }

  double value = 0.0;
  const char *first = buffer.data();
  const char *last = first + buffer.size();
  const auto result = std::from_chars(first, last, value);
  if (result.ec != std::errc{} || result.ptr != last) {
    return std::nullopt;
  }
  return value;
}

std::optional<StringView> StringLit::getValue() const {
  for (const auto &child : node.getChildrenWithTokens()) {
    if (!child.isToken()) {
      continue;
    }
    const auto &token = child.getToken();
    switch (token.getKind()) {
    case SyntaxKind::StringLiteral:
    case SyntaxKind::RawStringLiteral:
      return token.getGreen().getSource();
    default:
      break;
    }
  }
  return std::nullopt;
}

std::optional<Bool> StringLit::getIsRaw() const {
  for (const auto &child : node.getChildrenWithTokens()) {
    if (!child.isToken()) {
      continue;
    }
    switch (child.getToken().getKind()) {
    case SyntaxKind::StringLiteral:
      return false;
    case SyntaxKind::RawStringLiteral:
      return true;
    default:
      break;
    }
  }
  return std::nullopt;
}

} // namespace yuzu::ast
