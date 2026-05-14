#include "yuzu/Driver/CompilePipeline.h"

#include "llvm/Support/raw_ostream.h"
#include <gtest/gtest.h>
#include <string>

namespace {

yuzu::CompileOptions opts(llvm::raw_ostream &os) {
  return yuzu::CompileOptions{
      .out = os,
      .execute = true,
      .debugLexer = false,
      .debugAst = false,
      .debugHir = false,
      .debugMlir = false,
  };
}

} // namespace

TEST(CompilePipelineTest, OnePlusTwoExecutesToThree) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 + 2", opts(os));
  EXPECT_NE(out.find("=== result ===\n3\n"), std::string::npos) << out;
}

TEST(CompilePipelineTest, NestedPrecedence) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 + 2 * 3", opts(os));
  EXPECT_NE(out.find("=== result ===\n7\n"), std::string::npos) << out;
}

TEST(CompilePipelineTest, IncompleteBinaryStopsBeforeCodegen) {
  std::string out;
  llvm::raw_string_ostream os(out);
  yuzu::compile(U"1 +", opts(os));
  // Diagnostics emitted, no result section.
  EXPECT_EQ(out.find("=== result ==="), std::string::npos) << out;
}
