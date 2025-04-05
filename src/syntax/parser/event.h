#ifndef SYNTAX_PARSER_EVENT_H_
#define SYNTAX_PARSER_EVENT_H_

#include <optional>
#include <utility>
#include <variant>

#include "syntax/syntax_kind.h"

namespace orion::syntax {
/// \brief Represents an event in the syntax parser event stream.
/// Each event is a tagged variant indicating parser actions such as starting a
/// node, finishing a node, adding a token, or reporting an error.
class Event {
 public:
  /// \brief Marks the beginning of a syntax node.
  /// Contains the kind of the node and an optional forward parent index,
  /// which allows referencing a parent node that hasn't yet been completed.
  class Start {
   public:
    explicit Start(SyntaxKind kind, std::optional<size_t> forward_parent)
        : kind_(kind), forward_parent_(forward_parent) {}

    /// \return The kind of the syntax node being started.
    [[nodiscard]] SyntaxKind Kind() const noexcept { return this->kind_; }

    /// \return An optional index to a forward parent node.
    [[nodiscard]] std::optional<size_t> ForwardParent() const noexcept {
      return this->forward_parent_;
    }

   private:
    SyntaxKind kind_;
    std::optional<size_t> forward_parent_;
  };

  /// \brief Marks the end of a syntax node.
  class Finish {};

  /// \brief Represents a single token in the syntax stream.
  class Token {};

  /// \brief Represents a parse error.
  class Error {};

  /// \brief Placeholder event used as a sentinel or to overwrite used events.
  class Placeholder {};

 public:
  /// \brief Creates a Start event.
  ///
  /// This method constructs an `Event` of type `Start` with the specified
  /// syntax kind and an optional forward parent index.
  ///
  /// \param kind The `SyntaxKind` representing the type of the syntax node.
  /// \param forward_parent An optional index to a forward parent node that
  /// may not have been completed yet.
  /// \return A new `Event` representing the Start event.
  static Event CreateStart(
      const SyntaxKind kind,
      const std::optional<size_t> forward_parent = std::nullopt) {
    return Event(Event::Start{kind, forward_parent});
  }

  /// \brief Creates a Finish event.
  ///
  /// This method constructs an `Event` of type `Finish`, indicating the
  /// completion of a syntax node.
  ///
  /// \return A new `Event` representing the Finish event.
  static Event CreateFinish() { return Event(Event::Finish{}); }

  /// \brief Creates a Token event.
  ///
  /// This method constructs an `Event` of type `Token`, which represents
  /// a single token in the syntax tree.
  ///
  /// \return A new `Event` representing the Token event.
  static Event CreateToken() { return Event(Event::Token{}); }

  /// \brief Creates an Error event.
  ///
  /// This method constructs an `Event` of type `Error`, which indicates
  /// that an error has occurred during parsing.
  ///
  /// \return A new `Event` representing the Error event.
  static Event CreateError() { return Event(Event::Error{}); }

  /// \brief Creates a placeholder event.
  /// Used to mark an event slot as processed or irrelevant.
  static Event CreatePlaceholder() { return Event(Event::Placeholder{}); }

  /// \brief Checks whether the stored event is of the given alternative type.
  /// \tparam T The alternative type to check for (e.g., Start, Finish).
  /// \return True if the variant holds the given type, false otherwise.
  template <typename T>
  [[nodiscard]] bool HoldsAlternative() const noexcept {
    return std::holds_alternative<T>(variant_);
  }

  /// \brief Retrieves the stored event value as the specified type.
  /// \tparam T The alternative type to retrieve.
  /// \return The stored value as type T.
  /// \throws std::bad_variant_access if the type does not match.
  template <typename T>
  [[nodiscard]] T Get() const {
    return std::get<T>(variant_);
  }

 private:
  /// \brief Constructs an Event by wrapping the specified alternative.
  explicit Event(std::variant<Start, Finish, Token, Error, Placeholder> variant)
      : variant_(std::move(variant)) {}

  /// \brief Internal container to wrap different Event types.
  std::variant<Start, Finish, Token, Error, Placeholder> variant_;
};
}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_EVENT_H_
