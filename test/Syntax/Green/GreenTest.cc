#include "Syntax/Green/Green.h"

#include <gtest/gtest.h>

namespace {
using yuzu::syntax::GreenElement;
using yuzu::syntax::GreenNode;
using yuzu::syntax::GreenNodeData;
using yuzu::syntax::GreenToken;
using yuzu::syntax::GreenTokenData;

TEST(GreenElementTest, GreenElementSizeRequirements) {
  // std::variant:
  //   shared_ptr:
  //     pointer    = 8
  //     ref_count  = 8
  // index          = 4
  // alignment      = 4
  EXPECT_EQ(24, sizeof(GreenElement));
}

TEST(GreenNodeTest, GreenNodeSizeRequirements) {
  // shared_ptr:
  //   pointer    = 8
  //   ref_count  = 8
  EXPECT_EQ(16, sizeof(GreenNode));
}

TEST(GreenNodeTest, GreenNodeDataSizeRequirements) {
  // kind           = 2
  // alignment      = 6
  // width          = 8
  // std::vector    = 24
  EXPECT_EQ(40, sizeof(GreenNodeData));
}

TEST(GreenTokenTest, GreenTokenSizeRequirements) {
  // shared_ptr:
  //   pointer    = 8
  //   ref_count  = 8
  EXPECT_EQ(16, sizeof(GreenToken));
}

TEST(GreenTokenTest, GreenTokenDataSizeRequirements) {
  // kind                 = 2
  // alignment            = 6
  // std::u32string_view  = 32
  EXPECT_EQ(40, sizeof(GreenTokenData));
}
} // namespace
