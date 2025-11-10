#ifndef YUZU_PARSER_MARKER_H
#define YUZU_PARSER_MARKER_H

#include <cstddef>

namespace yuzu::parser {
/// \brief Represents a completed parsing operation.
///
/// CompletedMarker is returned when a syntax node has been successfully
/// parsed and finished. It records the position in the event stream where
/// the node was started, allowing for post-processing and tree construction.
struct CompletedMarker final {
  /// The position in the event stream where this node was started.
  const size_t position;
};

/// \brief Represents an active parsing operation.
///
/// Marker tracks the position in the event stream where a syntax node
/// begins. It is used during parsing to mark the start of a node before
/// its contents are parsed. Once parsing completes, it can be converted
/// to a CompletedMarker.
struct Marker final {
  /// The position in the event stream where this node starts.
  const size_t position;
};
} // namespace yuzu::parser

#endif // YUZU_PARSER_MARKER_H
