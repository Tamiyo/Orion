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
} // namespace yuzu::syntax
