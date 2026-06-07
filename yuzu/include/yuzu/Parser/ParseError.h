#ifndef YUZU_PARSER_PARSE_ERROR_H
#define YUZU_PARSER_PARSE_ERROR_H

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Diagnostics/Span.h"
#include "yuzu/Lexer/Range.h"
#include "yuzu/Lexer/TokenKind.h"
#include "yuzu/Util/Unicode.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace yuzu::parser {
/// \brief Abstract base for the parser's structured error types.
///
/// The parser stays unaware of the diagnostics layer at the call site —
/// it constructs `ParseError` subclasses and stuffs them into
/// `ErrorEvent`s. The `TokenSink` later asks each `ParseError` to render
/// itself as a `diagnostics::Diagnostic` (via `toDiagnostic`) and pushes
/// the result onto the diagnostics.
///
/// Adding a new error category means: subclass, hold the data the parser
/// captured, implement `toDiagnostic` to map that data to a primary span
/// + message (and any labels/notes you want).
class [[nodiscard]] ParseError {
public:
  virtual ~ParseError() = default;

  /// \brief Render this parse error as a renderable diagnostic.
  ///
  /// `source` is the `SourceId` of the source the parser was running
  /// against — the subclass uses it together with whatever byte-range
  /// it captured to build the diagnostic's `Span`.
  [[nodiscard]] virtual diagnostics::Diagnostic
  toDiagnostic(diagnostics::SourceId source) const = 0;
};

/// \brief Parser hit an unexpected token (or end-of-input).
///
/// `expected` lists every kind the parser would have accepted at this
/// decision point (collected via `Parser::at`); `found` is what was
/// actually there, or `nullopt` at EOF; `range` is the offending span
/// inside the source.
class [[nodiscard]] ExpectedKindError final : public ParseError {
public:
  explicit ExpectedKindError(std::vector<lexer::TokenKind> expected,
                             std::optional<lexer::TokenKind> found,
                             lexer::Range range)
      : expected(std::move(expected)), found(std::move(found)),
        range(std::move(range)) {}

  ExpectedKindError() = delete;

  [[nodiscard]] diagnostics::Diagnostic
  toDiagnostic(diagnostics::SourceId source) const override {
    const std::string foundName =
        found ? lexer::asDisplayString(*found) : std::string("end of input");

    std::string expectedList;
    for (size_t i = 0; i < expected.size(); ++i) {
      if (i > 0) {
        expectedList.append(", ");
      }
      expectedList.append(lexer::asDisplayString(expected[i]));
    }

    const std::string message =
        expected.size() == 1
            ? "expected " + expectedList + ", found " + foundName
            : "expected one of " + expectedList + ", found " + foundName;

    const diagnostics::Span span{source, range.start, range.end};
    return diagnostics::Diagnostic{
        .severity = diagnostics::Severity::Error,
        .code = "",
        .message = message,
        .labels = {diagnostics::Label{
            diagnostics::LabelStyle::Primary,
            span,
            "",
        }},
        .notes = {},
    };
  }

private:
  std::vector<lexer::TokenKind> expected;
  std::optional<lexer::TokenKind> found;
  lexer::Range range;
};

/// \brief Parser hit something other than an expression where one was
/// required.
///
/// Higher-level than `ExpectedKindError`: instead of enumerating the
/// kinds the parser would have accepted (Number, Ident, LeftParen),
/// this carries the semantic concept "expression". The diagnostic reads
/// `expected expression, found `<text>`` for tokens, or
/// `expected expression, found end of input` at EOF.
///
/// `foundText` is the UTF-32 source slice of the offending token (empty
/// on EOF) so the renderer can quote what the user actually typed
/// rather than print the lexer's recovery placeholder name.
class [[nodiscard]] ExpectedExpressionError final : public ParseError {
public:
  explicit ExpectedExpressionError(std::u32string foundText, lexer::Range range)
      : foundText(std::move(foundText)), range(std::move(range)) {}

  ExpectedExpressionError() = delete;

  [[nodiscard]] diagnostics::Diagnostic
  toDiagnostic(diagnostics::SourceId source) const override {
    const std::string foundDescription =
        foundText.empty() ? std::string("end of input")
                          : "`" + util::toUtf8(foundText) + "`";

    const diagnostics::Span span{source, range.start, range.end};
    return diagnostics::Diagnostic{
        .severity = diagnostics::Severity::Error,
        .code = "",
        .message = "expected expression, found " + foundDescription,
        .labels = {diagnostics::Label{
            diagnostics::LabelStyle::Primary,
            span,
            "",
        }},
        .notes = {},
    };
  }

private:
  std::u32string foundText;
  lexer::Range range;
};

} // namespace yuzu::parser

#endif // YUZU_PARSER_PARSE_ERROR_H
