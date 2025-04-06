#ifndef SYNTAX_LEXER_H_
#define SYNTAX_LEXER_H_

#include <optional>
#include <string>

#include "syntax/lexer/lexer_base.h"
#include "syntax/lexer/token.h"

namespace orion::syntax {

/// \brief The Lexer class is a concrete implementation of the LexerBase base
/// class.
///
/// The `Lexer` is responsible for tokenizing source code into meaningful
/// tokens that can be further processed by a parser. It implements the
/// tokenization logic specific to the Orion language syntax.
class Lexer final : public LexerBase {
 public:
  /// \brief Constructs an Lexer with the given source string.
  ///
  /// \param source The source string to be tokenized.
  explicit Lexer(std::u32string_view source) : LexerBase(source) {}

  /// \brief Deleted default constructor.
  ///
  /// An `Lexer` must always be initialized with a source string.
  Lexer() = delete;

 protected:
  /// \brief Attempts to retrieve the next token from the source.
  ///
  /// This method implements the logic to identify and create tokens from the
  /// source string, using various tokenization strategies.
  ///
  /// \return An optional containing the next token if successful, otherwise
  /// `nullopt`.
  std::optional<Token> TryNextToken() noexcept override;

 private:
  // Tokenization methods for different types of tokens.

  /// \brief Attempts to parse a whitespace token.
  ///
  /// \return An optional containing the whitespace token if found, otherwise
  /// `nullopt`.
  std::optional<Token> TryWhitespace();

  /// \brief Attempts to parse an operator token (e.g., `+`, `-`, etc.).
  ///
  /// \return An optional containing the operator token if found, otherwise
  /// `nullopt`.
  std::optional<Token> TryOperator();

  /// \brief Attempts to parse a keyword or an identifier token.
  ///
  /// \return An optional containing the keyword or identifier token if found,
  /// otherwise `nullopt`.
  std::optional<Token> TryKeywordOrIdentifier();

  /// \brief Attempts to parse a quoted identifier token.
  ///
  /// \return An optional containing the quoted identifier token if found,
  /// otherwise `nullopt`.
  std::optional<Token> TryQuotedIdentifier();

  /// \brief Attempts to parse a plain identifier token.
  ///
  /// \return An optional containing the identifier token if found, otherwise
  /// `nullopt`.
  std::optional<Token> TryIdentifier();

  /// \brief Attempts to parse a literal token (e.g., strings, booleans,
  /// numbers).
  ///
  /// \return An optional containing the literal token if found, otherwise
  /// `nullopt`.
  std::optional<Token> TryLiteral();

  /// \brief Attempts to parse a string literal token.
  ///
  /// \return An optional containing the string literal token if found,
  /// otherwise `nullopt`.
  std::optional<Token> TryStringLiteral();

  /// \brief Attempts to parse a boolean literal token.
  ///
  /// \return An optional containing the boolean literal token if found,
  /// otherwise `nullopt`.
  std::optional<Token> TryBooleanLiteral();

  /// \brief Attempts to parse a numeric literal token.
  ///
  /// \param consume_digits A flag indicating whether to consume digits after
  /// parsing. \return An optional containing the numeric literal token if
  /// found, otherwise `nullopt`.
  std::optional<Token> TryNumericLiteral(bool consume_digits = true);

  // Fragment handling methods for specific parts of tokens.

  /// \brief Bumps the exponent part of a numeric literal if present.
  void BumpExponent();

  /// \brief Bumps digits from the source string.
  void BumpDigits();

  /// \brief Bumps letters from the source string.
  void BumpLetters();
};

}  // namespace orion::syntax

#endif  // SYNTAX_LEXER_H_
