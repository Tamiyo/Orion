#ifndef YUZU_LEXER_TOKEN_H
#define YUZU_LEXER_TOKEN_H

#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/TokenKind.h"

#include <cstdint>
#include <string_view>
#include <utility>

namespace yuzu::lexer {
/// \brief Represents a lexical token in the source code.
///
/// Token encapsulates a single lexical element recognized by the lexer,
/// including its kind (e.g., keyword, identifier, operator), its location
/// in the source text, and a view of the source text it represents.
class [[nodiscard]] Token final {
public:
  /// \brief Construct a token.
  ///
  /// \param kind The kind of token (keyword, identifier, operator, etc.).
  /// \param source A view of the entire source text.
  /// \param range The range in the source text that this token occupies.
  explicit Token(TokenKind kind, std::u32string_view source,
                 const Range &range)
      : source(std::move(source)), range(std::move(range)), kind(kind) {}

  /// \brief Construct a token.
  ///
  /// \param kind The kind of token (keyword, identifier, operator, etc.).
  /// \param source A view of the entire source text.
  /// \param start The start of the token in source text the token occupies.
  /// \param end The end of the token in source text the token occupies.
  explicit Token(TokenKind kind, std::u32string_view source,
                 const uint32_t start, const uint32_t end)
      : source(std::move(source)), range(Range{.start = start, .end = end}),
        kind(kind) {}

  /// Deleted default constructor to enforce proper initialization.
  Token() = delete;

  /// \brief Get the kind of this token.
  ///
  /// \return The TokenKind of this token.
  [[nodiscard]] TokenKind getKind() const { return kind; }

  /// \brief Get the range of this token in the source text.
  ///
  /// \return Constant reference to the Range.
  [[nodiscard]] const Range &getRange() const { return range; }

  /// \brief Get the source text view.
  ///
  /// \return A view of the entire source text.
  [[nodiscard]] std::u32string_view getSource() const { return source; }

  /// \brief Get the length of this token.
  ///
  /// \return The length in characters (end - start).
  [[nodiscard]] uint32_t getLength() const { return range.end - range.start; }

  /// \brief Equality comparison operator.
  ///
  /// \param other The Token to compare with.
  /// \return True if both tokens have the same kind, source, and range.
  bool operator==(const Token &other) const {
    return kind == other.kind && source == other.source && range == other.range;
  }

private:
  /// A view of the entire source text.
  const std::u32string_view source;

  /// The range in the source text that this token occupies.
  const Range range;

  /// The kind of this token.
  const TokenKind kind;
};
} // namespace yuzu::lexer

#endif // YUZU_LEXER_TOKEN_H
