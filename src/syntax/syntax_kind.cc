#include "syntax/syntax_kind.h"

#include <codecvt>
#include <locale>
#include <sstream>
#include <string_view>

namespace orion::syntax {
constexpr std::u32string_view SyntaxKindToString(
    const SyntaxKind kind) noexcept {
  switch (kind) {
    case SyntaxKind::kWhitespace:
      return U"Whitespace";
    case SyntaxKind::kNewline:
      return U"Newline";
    case SyntaxKind::kComment:
      return U"Comment";

    case SyntaxKind::kDot:
      return U"Dot";
    case SyntaxKind::kPlus:
      return U"Plus";
    case SyntaxKind::kMinus:
      return U"Minus";
    case SyntaxKind::kAsterisk:
      return U"Asterisk";
    case SyntaxKind::kSlash:
      return U"Slash";
    case SyntaxKind::kPercent:
      return U"Percent";

    case SyntaxKind::kBooleanLiteral:
      return U"BooleanLiteral";

    case SyntaxKind::kStringLiteral:
      return U"StringLiteral";

    case SyntaxKind::kIntLiteral:
      return U"IntLiteral";
    case SyntaxKind::kBigIntLiteral:
      return U"BigIntLiteral";
    case SyntaxKind::kSmallIntLiteral:
      return U"SmallIntLiteral";
    case SyntaxKind::kTinyIntLiteral:
      return U"TinyIntLiteral";

    case SyntaxKind::kFloatLiteral:
      return U"FloatLiteral";
    case SyntaxKind::kDoubleLit:
      return U"DoubleLit";
    case SyntaxKind::kBigDecimalLiteral:
      return U"BigDecimalLiteral";

    case SyntaxKind::kIdentifier:
      return U"Identifier";
    case SyntaxKind::kQuotedIdentifier:
      return U"QuotedIdentifier";

    case SyntaxKind::kEof:
      return U"Eof";

    case SyntaxKind::kRoot:
      return U"Root";

    case SyntaxKind::kBinaryExpr:
      return U"BinaryExpr";

    case SyntaxKind::kError:
      return U"Error";

    default:
      return U"Unknown";
  }
}

std::basic_ostringstream<char32_t>& operator<<(
    std::basic_ostringstream<char32_t>& oss, const SyntaxKind kind) noexcept {
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> conv;
  oss << SyntaxKindToString(kind);
  return oss;
}
};  // namespace orion::syntax
