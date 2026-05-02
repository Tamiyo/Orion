yuzu_tablegen(YuzuLexerTokenKindInc "yuzu/Lexer/TokenKind.h.inc" -gen-token-kind-decls)

add_library(YuzuLexer Lexer.cc)
add_dependencies(YuzuLexer YuzuLexerTokenKindInc)
target_link_libraries(YuzuLexer PUBLIC YuzuUtil)
