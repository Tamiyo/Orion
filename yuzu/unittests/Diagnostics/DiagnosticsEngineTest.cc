#include "yuzu/Diagnostics/DiagnosticsEngine.h"

#include "yuzu/Diagnostics/Diagnostic.h"
#include "yuzu/Diagnostics/DiagnosticBuilder.h"
#include "yuzu/Diagnostics/Span.h"

#include <gtest/gtest.h>

namespace {
using yuzu::diagnostics::DiagnosticsEngine;
using yuzu::diagnostics::LabelStyle;
using yuzu::diagnostics::Severity;
using yuzu::diagnostics::SourceId;
using yuzu::diagnostics::Span;

Span span(uint32_t start, uint32_t end) {
  return Span{SourceId{1}, start, end};
}

TEST(DiagnosticsEngineTest, StartsEmpty) {
  DiagnosticsEngine engine;

  EXPECT_TRUE(engine.getDiagnostics().empty());
  EXPECT_EQ(0u, engine.getErrorCount());
  EXPECT_EQ(0u, engine.getWarningCount());
  EXPECT_FALSE(engine.hasErrors());
}

TEST(DiagnosticsEngineTest, ErrorEmitsWithPrimaryLabel) {
  DiagnosticsEngine engine;
  engine.error(span(0, 1), "expected expression").emit();

  ASSERT_EQ(1u, engine.getDiagnostics().size());
  const auto &d = engine.getDiagnostics()[0];
  EXPECT_EQ(Severity::Error, d.severity);
  EXPECT_EQ("expected expression", d.message);
  EXPECT_TRUE(d.code.empty());

  // The engine seeds the builder with one Primary label spanning the
  // diagnostic site; callers add more via `.label(...)`.
  ASSERT_EQ(1u, d.labels.size());
  EXPECT_EQ(LabelStyle::Primary, d.labels[0].style);
  EXPECT_EQ(span(0, 1), d.labels[0].span);
  EXPECT_EQ("", d.labels[0].message);
}

TEST(DiagnosticsEngineTest, BuilderChainsCodeLabelsAndNotes) {
  DiagnosticsEngine engine;
  engine.error(span(2, 3), "expected expression")
      .code("E0001")
      .label(span(0, 1), "after this `+`")
      .note("expressions can start with a number, identifier, or `(`")
      .emit();

  ASSERT_EQ(1u, engine.getDiagnostics().size());
  const auto &d = engine.getDiagnostics()[0];
  EXPECT_EQ("E0001", d.code);

  ASSERT_EQ(2u, d.labels.size());
  EXPECT_EQ(LabelStyle::Primary, d.labels[0].style);
  EXPECT_EQ(span(2, 3), d.labels[0].span);
  EXPECT_EQ(LabelStyle::Secondary, d.labels[1].style);
  EXPECT_EQ(span(0, 1), d.labels[1].span);
  EXPECT_EQ("after this `+`", d.labels[1].message);

  ASSERT_EQ(1u, d.notes.size());
  EXPECT_EQ("expressions can start with a number, identifier, or `(`",
            d.notes[0]);
}

TEST(DiagnosticsEngineTest, CountsTrackSeverity) {
  DiagnosticsEngine engine;
  engine.error(span(0, 1), "first").emit();
  engine.warning(span(1, 2), "second").emit();
  engine.error(span(2, 3), "third").emit();
  engine.remark(span(3, 4), "fourth").emit();

  EXPECT_EQ(4u, engine.getDiagnostics().size());
  EXPECT_EQ(2u, engine.getErrorCount());
  EXPECT_EQ(1u, engine.getWarningCount());
  EXPECT_TRUE(engine.hasErrors());
}

TEST(DiagnosticsEngineTest, BuilderWithoutEmitDropsDiagnostic) {
  DiagnosticsEngine engine;
  // Build but never call .emit() — the diagnostic is silently dropped.
  // Useful for recovery code that decides late not to surface an error.
  // Cast to void to acknowledge the [[nodiscard]] return.
  (void)engine.error(span(0, 1), "nevermind");

  EXPECT_TRUE(engine.getDiagnostics().empty());
}

TEST(DiagnosticsEngineTest, PeekExposesInProgressDiagnostic) {
  DiagnosticsEngine engine;
  // Move-initialize from the engine factory's return (prvalue), then
  // chain via the lvalue. Chaining straight into `auto builder = ...`
  // would copy from the lvalue reference returned by `code()`, which is
  // deleted.
  auto builder = engine.error(span(0, 1), "msg");
  builder.code("E0001");

  // `peek()` lets tests inspect a builder without committing it.
  EXPECT_EQ("msg", builder.peek().message);
  EXPECT_EQ("E0001", builder.peek().code);
  EXPECT_EQ(Severity::Error, builder.peek().severity);
}

TEST(DiagnosticsEngineTest, PrimaryLabelAddsAdditionalPrimary) {
  // The first call to engine.error() seeds one Primary label. Callers
  // can promote a second span to Primary status with `.primaryLabel`.
  DiagnosticsEngine engine;
  engine.error(span(0, 1), "msg")
      .primaryLabel(span(5, 7), "and here")
      .emit();

  const auto &d = engine.getDiagnostics()[0];
  ASSERT_EQ(2u, d.labels.size());
  EXPECT_EQ(LabelStyle::Primary, d.labels[0].style);
  EXPECT_EQ(LabelStyle::Primary, d.labels[1].style);
  EXPECT_EQ("and here", d.labels[1].message);
}

} // namespace
