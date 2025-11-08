#ifndef YUZU_PARSING_TOKEN_H
#define YUZU_PARSING_TOKEN_H

#include "yuzu/Parsing/Span.h"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <utility>

namespace yuzu::parsing {
/// \brief Represents the different kinds of tokens in the lexer.
///
/// This enum class categorizes various token types encountered during
/// lexical analysis, including literals, operators, punctuation, and keywords.
enum class TokenKind : uint16_t {
  // --- Trivia ---
  Whitespace, /// Whitespace (e.g. spaces, tabs).
  Newline,    /// Newline characters.
  Comment,    /// Line or block comments.

  // --- Keywords ---
  // (To be added.)

  // --- Punctuation ---
  Dot,         /// Dot or period (e.g. `.`).
  Plus,        /// Plus sign (e.g. `+`).
  Minus,       /// Minus sign (e.g. `-`).
  Asterisk,    /// Asterisk or multiplication sign (e.g. `*`).
  Slash,       /// Slash or division operator (e.g. `/`).
  Percent,     /// Percent or modulo operator (e.g. `%`).
  LeftParen,   /// Left parenthesis (e.g. `(`).
  RightParen,  /// Right parenthesis (e.g. `)`).
  LeftSquare,  /// Left square bracket (e.g. `[`).
  RightSquare, /// Right square bracket (e.g. `]`).

  // --- Boolean Literals ---
  BooleanLit, /// A boolean literal (`true`, `false`).

  // --- String Literals ---
  StringLit, /// A string literal (e.g. `"hello"`).

  // --- Exact Numeric Literals ---
  BigDecimalLit, /// A arbitrary-precision signed decimal number.
  BigIntLit,     /// A 64-bit (4 byte) big integer literal.
  IntLit,        /// A 32-bit (3 byte) integer literal.
  SmallIntLit,   /// A 16-bit (2 byte) small integer literal.
  TinyIntLit,    /// A 8-bit (1 byte) tiny integer literal.

  // --- Approx Numeric Literals ---
  FloatLit,  /// A floating-point number literal.
  DoubleLit, /// A double precision floating-point literal.

  // --- Other ---
  UnquotedIdent, /// An unquoted identifier (e.g. variable name).
  QuotedIdent,   /// A quoted identifier (e.g. `"column"`).

  // --- Special ---
  Eof, /// End-of-file marker.
};

constexpr bool IsTrivia(const TokenKind kind) noexcept {
  switch (kind) {
  case TokenKind::Whitespace:
  case TokenKind::Newline:
  case TokenKind::Comment:
    return true;
  default:
    return false;
  }
};

class Token final {
public:
  explicit Token(TokenKind Kind, const std::u32string_view &Source,
                 const Span &Span)
      : Source_(std::move(Source)), Span_(std::move(Span)), Kind_(Kind) {}

  Token() = delete;

  [[nodiscard]] TokenKind getKind() const noexcept { return Kind_; }

  [[nodiscard]] const Span &getSpan() const noexcept { return Span_; }

  [[nodiscard]] std::u32string_view getSource() const noexcept {
    return Source_;
  }

  [[nodiscard]] size_t getLength() const noexcept {
    return Span_.End - Span_.Start;
  }

  bool operator==(const Token &Other) const noexcept {
    return Kind_ == Other.Kind_ && Source_ == Other.Source_ &&
           Span_ == Other.Span_;
  }

private:
  const std::u32string_view Source_;
  const Span Span_;
  const TokenKind Kind_;
};
} // namespace yuzu::parsing

#endif // YUZU_PARSING_TOKEN_H
