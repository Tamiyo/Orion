#include "yuzu/Syntax/Green/GreenBuilder.h"

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Syntax/Green/GreenCache.h"
#include "yuzu/Syntax/SyntaxKind.h"
#include "yuzu/Util/ErrorHandling.h"

#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace yuzu::syntax {
namespace {
constexpr size_t maxCachedNodeSize = 3;
} // namespace

GreenBuilder::GreenBuilder() : cache(GreenCache(maxCachedNodeSize)) {}

GreenBuilder::GreenBuilder(const size_t maxCachedNodeSize)
    : cache(GreenCache(maxCachedNodeSize)) {}

void GreenBuilder::startNode(const SyntaxKind kind) noexcept {
  parents.emplace_back(
      GreenBuilder::Parent{.kind = kind, .firstChild = children.size()});
}

void GreenBuilder::finishNode() noexcept {
  // Finishing a node requires a parent.
  if (parents.empty()) {
    util::yuzu_unreachable();
  }

  const auto [kind, firstChild] = parents.back();
  parents.pop_back();

  const auto entry = cache.getNode(kind, &children, firstChild);
  children.emplace_back(entry);
}

void GreenBuilder::startNodeAt(const GreenBuilderCheckpoint &checkpoint,
                               const SyntaxKind kind) noexcept {
  // Checkpoints should never reference elements outside of Children.
  if (checkpoint.index >= children.size()) {
    util::yuzu_unreachable();
  }

  if (!parents.empty()) {
    // Checkpoints should never reference elements prior to the current Parent.
    if (const GreenBuilder::Parent parent = parents.back();
        checkpoint.index < parent.firstChild) {
      util::yuzu_unreachable();
    }
  }

  parents.emplace_back(
      GreenBuilder::Parent{.kind = kind, .firstChild = checkpoint.index});
}

GreenBuilderCheckpoint GreenBuilder::checkpoint() const noexcept {
  return GreenBuilderCheckpoint{.index = children.size()};
}

void GreenBuilder::token(const SyntaxKind kind,
                         const std::u32string_view &source) noexcept {
  const auto token = cache.getToken(kind, source);
  children.emplace_back(token);
}

GreenNode GreenBuilder::finish() noexcept {
  // Finishing building requires a parent.
  if (!parents.empty()) {
    util::yuzu_unreachable("GreenBuilder parents empty");
  }

  const GreenCacheEntry entry = children.back();
  children.pop_back();

  // The last entry should be a Node, Tokens cannot represent finished Green
  // trees.
  if (const GreenNode *node = entry.element.getIfNode()) {
    return std::move(*node);
  } else {
    util::yuzu_unreachable();
  }
}

} // namespace yuzu::syntax
