#ifndef SYNTAX_LEXER_LEXER_H_
#define SYNTAX_LEXER_LEXER_H_

#include <cstddef>
#include <functional>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

#include "syntax/lexer/token.h"

namespace yuzu::syntax {

/// \brief Represents a base class for a lexer that tokenizes source code.
///
/// The `Lexer` class provides a framework for tokenizing a source string
/// into a sequence of tokens. Derived classes must implement the `TryNextToken`
/// method to provide tokenization logic.
template <typename TokenKind = uint16_t>
class Lexer {
 public:
  using Token = Token<TokenKind>;

  /// \brief Deleted default constructor.
  ///
  /// A `Lexer` must always be initialized with a source string.
  Lexer() = delete;

  /// \brief Default destructor.
  virtual ~Lexer() = default;

  /// \brief Tokenizes the entire source string.
  ///
  /// \return A vector of tokens generated from the source.
  std::vector<Token> Tokenize() noexcept {
    auto tokens = std::vector<Token>();
    while (!AtEnd()) {
      if (const std::optional<Token> token = TryNextToken();
          token.has_value()) {
        tokens.emplace_back(token.value());
      } else {
        break;
      }
    }

    return tokens;
  }

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
  [[nodiscard]] Token CreateToken(const TokenKind kind) noexcept {
    const size_t distance = end_ - start_;
    const std::u32string source = source_.substr(start_, distance);
    const auto span = Span(start_, end_);
    const auto token = Token(kind, span, source);

    start_ = end_;
    return token;
  }

  /// \brief Bumps characters and creates a token from the current state.
  ///
  /// \tparam TokenKind The type of the token kind (defaults to `uint16_t`).
  /// \param kind The kind of the token to create.
  /// \param count The number of characters to consume (defaults to 1).
  /// \return A newly created token after consuming the specified characters.
  [[nodiscard]] Token BumpAndCreateToken(const TokenKind kind,
                                         const size_t count = 1) noexcept {
    Bump(count);
    return CreateToken(kind);
  }

  // State Management

  /// \brief Checks if the end of the source has been reached.
  ///
  /// \param offset An optional offset to check beyond the current position.
  /// \return `true` if the end of the source has been reached, otherwise
  /// `false`.
  [[nodiscard]] bool AtEnd(const size_t offset = 0) const noexcept {
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
  [[nodiscard]] bool At(const char32_t ch, const size_t offset = 0) const {
    if (AtEnd(offset)) {
      return false;
    }

    const size_t current = end_ + offset;
    return source_.at(current) == ch;
  }

  /// \brief Checks if the current characters match a specific string.
  ///
  /// \param value The string to check against.
  /// \param offset An optional offset from the current position.
  /// \return `true` if the current characters match the string, otherwise
  /// `false`.
  [[nodiscard]] bool At(std::u32string_view value,
                        const size_t offset = 0) const {
    if (AtEnd(offset + value.size() - 1)) {
      return false;
    }

    const auto substring =
        std::u32string_view(source_).substr(end_ + offset, value.size());

    return substring == value;
  }

  /// \brief Checks if the current character matches a predicate.
  ///
  /// \param predicate A function that takes a character and returns a boolean.
  /// \param offset An optional offset from the current position.
  /// \return `true` if the current character matches the predicate, otherwise
  /// `false`.
  [[nodiscard]] bool At(const std::function<bool(char32_t)>& predicate,
                        const size_t offset = 0) const {
    if (AtEnd(offset)) {
      return false;
    }

    const size_t current = end_ + offset;
    return predicate(source_.at(current));
  }

  /// \brief Checks if the current two characters match specific characters.
  ///
  /// \param ch1 The first character to check.
  /// \param ch2 The second character to check.
  /// \param offset An optional offset from the current position.
  /// \return `true` if both characters match, otherwise `false`.
  [[nodiscard]] bool At2(const char32_t ch1, const char32_t ch2,
                         const size_t offset = 0) const {
    return At(ch1, offset) || At(ch2, offset);
  }

  /// \brief Checks if the current three characters match specific characters.
  ///
  /// \param ch1 The first character to check.
  /// \param ch2 The second character to check.
  /// \param ch3 The third character to check.
  /// \param offset An optional offset from the current position.
  /// \return `true` if all three characters match, otherwise `false`.
  [[nodiscard]] bool At3(const char32_t ch1, const char32_t ch2,
                         const char32_t ch3, const size_t offset = 0) const {
    return At(ch1, offset) || At(ch2, offset) || At(ch3, offset);
  }

  // Bump

  /// \brief Bumps a specified number of characters from the source.
  ///
  /// \param count The number of characters to consume (defaults to 1).
  void Bump(const size_t count = 1) {
    size_t consumed = 0;
    while (!AtEnd() && consumed++ < count) {
      end_ += 1;
    }
  }

  /// \brief Bumps a character if a condition is met.
  ///
  /// \param condition A boolean condition to check before consuming.
  void BumpIf(const bool condition) {
    if (!AtEnd() && condition) {
      Bump();
    }
  }

  /// \brief Bumps characters while a predicate holds true.
  ///
  /// \param predicate A function that takes a character and returns a boolean.
  void BumpWhile(const std::function<bool(char32_t)>& predicate) {
    while (!AtEnd() && predicate(source_.at(end_))) {
      end_++;
    }
  }

  /// \brief Attempts to consume a specific character.
  ///
  /// \param ch The character to attempt to consume.
  void TryBump(const char32_t ch) {
    if (!AtEnd() && source_.at(end_) == ch) {
      Bump();
    }
  }

  /// \brief Attempts to consume two specific characters in sequence.
  ///
  /// \param ch1 The first character to attempt to consume.
  /// \param ch2 The second character to attempt to consume.
  void TryBump2(const char32_t ch1, const char32_t ch2) {
    if (!AtEnd() && (source_.at(end_) == ch1 || source_.at(end_) == ch2)) {
      Bump();
    }
  }

 private:
  const std::u32string source_;
  const size_t source_length_;
  size_t start_;
  size_t end_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_LEXER_LEXER_H_
