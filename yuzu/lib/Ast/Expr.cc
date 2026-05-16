#include "yuzu/Ast/Ast.h"

#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>

namespace yuzu::ast {
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
      default:
        break;
      }

      break;
    }
  }

  return std::nullopt;
}

namespace {

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

std::optional<Int64> IntLit::getValue() const {
  // Walk children-with-tokens; the first literal-shaped token wins.
  for (const auto &child : node.getChildrenWithTokens()) {
    if (!child.isToken()) {
      continue;
    }
    const auto &tok = child.getToken();
    const auto text = tok.getGreen().getSource();
    switch (tok.getKind()) {
    case SyntaxKind::IntegerLiteral:
      return parseIntegerLiteral(text, /*base=*/10, /*skip=*/0);
    case SyntaxKind::HexLiteral:
      return parseIntegerLiteral(text, /*base=*/16, /*skip=*/2);
    case SyntaxKind::BinaryLiteral:
      return parseIntegerLiteral(text, /*base=*/2, /*skip=*/2);
    case SyntaxKind::BooleanLiteral:
      // Temporary: booleans piggyback on `IntLit` until the AST grows a
      // dedicated `BoolLit` node. `true` -> 1, `false` -> 0.
      return text == U"true" ? Int64{1} : Int64{0};
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

std::optional<String> StringLit::getValue() const {
  // TODO: parse the StringLiteral token, strip quotes, unescape. Stubbed
  // out until codegen needs string values.
  return std::nullopt;
}

std::optional<Bool> StringLit::getIsRaw() const {
  // TODO: report whether the underlying token kind is RawStringLiteral.
  return std::nullopt;
}

} // namespace yuzu::ast
