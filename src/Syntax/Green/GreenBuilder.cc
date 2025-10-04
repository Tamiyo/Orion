#include "Syntax/Green/GreenBuilder.h"

#include "Syntax/Green/Green.h"
#include "Syntax/Green/GreenCache.h"
#include "Syntax/SyntaxKind.h"
#include "Util/ErrorHandling.h"

#include <memory>
#include <string>
#include <utility>

namespace yuzu::syntax {
namespace {
constexpr size_t kMaxNodeSize = 3;
} // namespace

GreenBuilder::GreenBuilder() : Cache_(GreenCache(kMaxNodeSize)) {}

GreenBuilder::GreenBuilder(const size_t MaxNodeSize)
    : Cache_(GreenCache(MaxNodeSize)) {}

void GreenBuilder::startNode(const SyntaxKind Kind) noexcept {
  Parents_.emplace_back(GreenBuilder::Parent{Kind, Children_.size()});
}

void GreenBuilder::finishNode() noexcept {
  if (Parents_.empty()) {
    util::yuzu_unreachable();
  }

  const auto [Kind, FirstChild] = Parents_.back();
  Parents_.pop_back();

  const auto Entry = Cache_.getNode(Kind, &Children_, FirstChild);

  Children_.emplace_back(Entry);
}

void GreenBuilder::startNodeAt(const GreenBuilderCheckpoint &Checkpoint,
                               const SyntaxKind Kind) noexcept {
  if (Checkpoint.Index > Children_.size()) {
    util::yuzu_unreachable();
  }

  if (!Parents_.empty()) {
    if (const GreenBuilder::Parent Parent = Parents_.back();
        Checkpoint.Index < Parent.FirstChild) {
      util::yuzu_unreachable();
    }
  }

  Parents_.emplace_back(GreenBuilder::Parent{Kind, Checkpoint.Index});
}

GreenBuilderCheckpoint GreenBuilder::checkpoint() const noexcept {
  return {Children_.size()};
}

void GreenBuilder::token(const SyntaxKind Kind,
                         const std::u32string &Source) noexcept {
  const auto Token = Cache_.getToken(Kind, Source);
  Children_.emplace_back(Token);
}

GreenNode GreenBuilder::finish() noexcept {
  if (!Parents_.empty()) {
    util::yuzu_unreachable();
  }

  const auto Entry = Children_.back();
  Children_.pop_back();

  if (const GreenNode *Node = std::get_if<GreenNode>(&Entry.Element)) {
    return *Node;
  } else {
    util::yuzu_unreachable();
  }
}

} // namespace yuzu::syntax
