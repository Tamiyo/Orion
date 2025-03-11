#ifndef SYNTAX_PARSER_SYNTAX_KIND_H_
#define SYNTAX_PARSER_SYNTAX_KIND_H_

#include <cstdint>

namespace orion::syntax {
enum class SyntaxKind : uint16_t {
    kPlus,
    kMinus,
    kError
};
}  // namespace orion::syntax

#endif  // SYNTAX_PARSER_RGTREE_SYNTAX_KIND_H_
