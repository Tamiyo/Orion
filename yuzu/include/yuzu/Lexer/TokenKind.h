#ifndef YUZU_LEXER_TOKEN_KIND_H
#define YUZU_LEXER_TOKEN_KIND_H

// These includes are consumed by TokenKind.h.inc; the generated file emits
// no includes of its own, so this wrapper stages the dependencies.
#include "yuzu/Util/ErrorHandling.h" // IWYU pragma: keep

#include <cstdint> // IWYU pragma: keep
#include <string>  // IWYU pragma: keep

#include "yuzu/Lexer/TokenKind.h.inc" // IWYU pragma: export

#endif // YUZU_LEXER_TOKEN_KIND_H
