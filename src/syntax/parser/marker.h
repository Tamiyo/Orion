#ifndef SYNTAX_PARSER_MARKER_H_
#define SYNTAX_PARSER_MARKER_H_

#include <cstddef>

namespace yuzu::syntax {

/// \brief Forward declaration of the Marker class.
class Marker;

/// \brief Represents a marker that has been completed in the event stream.
/// Used to keep track of the start position of a completed syntax node.
class CompletedMarker {
 public:
  /// \param position The index in the event stream where the node started.
  explicit CompletedMarker(size_t position) : position_(position) {}

  /// \return The starting position of the completed node.
  [[nodiscard]] size_t Position() const noexcept { return this->position_; }

 private:
  const size_t position_;
};

/// \brief Represents a marker for the start of a syntax node in the event
/// stream. This is used during parsing to record where a node begins.
class Marker {
 public:
  /// \param position The index in the event stream where the node starts.
  explicit Marker(size_t position) : position_(position) {}

  /// \return The starting position of the node.
  [[nodiscard]] size_t Position() const noexcept { return this->position_; }

 private:
  const size_t position_;
};

}  // namespace yuzu::syntax

#endif  // SYNTAX_PARSER_MARKER_H_
