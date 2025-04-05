#ifndef SYNTAX_LEXER_LEXER_H_
#define SYNTAX_LEXER_LEXER_H_

#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "syntax/lexer/token.h"

namespace orion::syntax {

/// \brief Represents a base class for a lexer that tokenizes source code.
///
/// The `Lexer` class provides a framework for tokenizing a source string into
/// a sequence of tokens. Derived classes must implement the `TryNextToken`
/// method to provide tokenization logic.
class Lexer {
 public:
  /// \brief Deleted default constructor.
  ///
  /// A `Lexer` must always be initialized with a source string.
  Lexer() = delete;

  /// \brief Default destructor.
  virtual ~Lexer() = default;

  /// \brief Tokenizes the entire source string.
  ///
  /// \return A vector of tokens generated from the source.
  std::vector<Token> Tokenize() noexcept;

 protected:
  /// \brief Constructs a `Lexer` with the specified source string.
  ///
  /// \param source The source string to be tokenized.
  explicit Lexer(std::u32string source)
      : source_(std::move(source)),
        source_length_(source_.length()),
        start_(0),
        end_(0) {}

  /// \brief Attempts to retrieve the next token from the source.
  ///
  /// This method must be implemented by derived classes to define specific
  /// tokenization behavior.
  ///
  /// \return An optional containing the next token if successful, otherwise
  /// `nullopt`.
  virtual std::optional<Token> TryNextToken() noexcept = 0;

  // Utils

  /// \brief Creates a token from the current source state.
  ///
  /// \tparam TokenKind The type of the token kind (defaults to `uint16_t`).
  /// \param kind The kind of the token to create.
  /// \return A newly created token.
  template <typename TokenKind = uint16_t>
  [[nodiscard]] Token CreateToken(TokenKind kind) noexcept {
    const size_t distance = end_ - start_;
    const std::u32string source = source_.substr(start_, distance);
    const auto span = Span(start_, end_);
    const auto token = Token(static_cast<uint16_t>(kind), span, source);

    start_ = end_;
    return token;
  }

  /// \brief Consumes characters and creates a token from the current state.
  ///
  /// \tparam TokenKind The type of the token kind (defaults to `uint16_t`).
  /// \param kind The kind of the token to create.
  /// \param count The number of characters to consume (defaults to 1).
  /// \return A newly created token after consuming the specified characters.
  template <typename TokenKind = uint16_t>
  [[nodiscard]] Token ConsumeAndCreateToken(TokenKind kind,
                                            const size_t count = 1) noexcept {
    Consume(count);
    return CreateToken<TokenKind>(kind);
  }

  // State Management

  /// \brief Checks if the end of the source has been reached.
  ///
  /// \param offset An optional offset to check beyond the current position.
  /// \return `true` if the end of the source has been reached, otherwise
  /// `false`.
  [[nodiscard]] bool AtEnd(size_t offset = 0) const noexcept {
    return end_ + offset >= source_length_;
  }

  // Peek

  /// \brief Returns the current character in the source.
  ///
  /// \return The character at the current position.
  [[nodiscard]] char32_t GetCurrent() const { return source_.at(end_); }

  // Check

  /// \brief Checks if the current character matches a specific character.
  ///
  /// \param ch The character to check.
  /// \param offset An optional offset from the current position.
  /// \return `true` if the current character matches, otherwise `false`.
  [[nodiscard]] bool At(char32_t ch, size_t offset = 0) const;

  /// \brief Checks if the current characters match a specific string.
  ///
  /// \param value The string to check against.
  /// \param offset An optional offset from the current position.
  /// \return `true` if the current characters match the string, otherwise
  /// `false`.
  [[nodiscard]] bool At(const std::u32string& value, size_t offset = 0) const;

  /// \brief Checks if the current character matches a predicate.
  ///
  /// \param predicate A function that takes a character and returns a boolean.
  /// \param offset An optional offset from the current position.
  /// \return `true` if the current character matches the predicate, otherwise
  /// `false`.
  [[nodiscard]] bool At(const std::function<bool(char32_t)>& predicate,
                        size_t offset = 0) const;

  /// \brief Checks if the current two characters match specific characters.
  ///
  /// \param ch1 The first character to check.
  /// \param ch2 The second character to check.
  /// \param offset An optional offset from the current position.
  /// \return `true` if both characters match, otherwise `false`.
  [[nodiscard]] bool At2(char32_t ch1, char32_t ch2, size_t offset = 0) const;

  /// \brief Checks if the current three characters match specific characters.
  ///
  /// \param ch1 The first character to check.
  /// \param ch2 The second character to check.
  /// \param ch3 The third character to check.
  /// \param offset An optional offset from the current position.
  /// \return `true` if all three characters match, otherwise `false`.
  [[nodiscard]] bool At3(char32_t ch1, char32_t ch2, char32_t ch3,
                         size_t offset = 0) const;

  // Consume

  /// \brief Consumes a specified number of characters from the source.
  ///
  /// \param count The number of characters to consume (defaults to 1).
  void Consume(size_t count = 1);

  /// \brief Consumes a character if a condition is met.
  ///
  /// \param condition A boolean condition to check before consuming.
  void ConsumeIf(bool condition);

  /// \brief Consumes characters while a predicate holds true.
  ///
  /// \param predicate A function that takes a character and returns a boolean.
  void ConsumeWhile(const std::function<bool(char32_t)>& predicate);

  /// \brief Attempts to consume a specific character.
  ///
  /// \param ch The character to attempt to consume.
  void TryConsume(char32_t ch);

  /// \brief Attempts to consume two specific characters in sequence.
  ///
  /// \param ch1 The first character to attempt to consume.
  /// \param ch2 The second character to attempt to consume.
  void TryConsume2(char32_t ch1, char32_t ch2);

 private:
  /// The source string being tokenized.
  const std::u32string source_;

  /// The length of the source string.
  const size_t source_length_;

  /// The current start position in the source for token creation.
  size_t start_;

  /// The current end position in the source for token creation.
  size_t end_;
};

}  // namespace orion::syntax

#endif  // SYNTAX_LEXER_LEXER_H_
