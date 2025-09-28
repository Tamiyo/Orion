#include "Syntax/SyntaxIterator.h"

#include "Syntax/Green/Green.h"
#include "Util/ErrorHandling.h"

#include <optional>
#include <vector>

namespace yuzu::syntax {
bool NoFilter::operator()(const GreenElement &) const { return true; }

bool NodeOnlyFilter::operator()(const GreenElement &Element) const {
  return Element.isNode();
}

template <typename ValueType, typename FilterPredicate>
ValueType SyntaxIterator<ValueType, FilterPredicate>::createElement() const {
  {
    if constexpr (std::is_same_v<ValueType, SyntaxNode>) {
      if (const std::optional<GreenNode> Node = It_->tryGetNode()) {
        return SyntaxNode(Offset_, &Parent_, Node.value());
      }

      util::yuzu_unreachable();
    }

    if constexpr (std::is_same_v<ValueType, SyntaxToken>) {
      if (const std::optional<GreenToken> Token = It_->tryGetToken()) {
        return SyntaxToken(Offset_, &Parent_, Token.value());
      }

      util::yuzu_unreachable();
    }

    if constexpr (std::is_same_v<ValueType, SyntaxElement>) {
      if (const std::optional<GreenNode> Node = It_->tryGetNode()) {
        return SyntaxNode(Offset_, &Parent_, Node.value());
      }

      if (const std::optional<GreenToken> Token = It_->tryGetToken()) {
        return SyntaxToken(Offset_, &Parent_, Token.value());
      }

      util::yuzu_unreachable();
    }
  }
}

} // namespace yuzu::syntax
