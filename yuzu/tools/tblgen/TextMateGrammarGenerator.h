#ifndef YUZU_TOOLS_TBLGEN_TEXTMATE_GRAMMAR_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_TEXTMATE_GRAMMAR_GENERATOR_H

#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>

namespace yuzu::tools {

/// Emits a TextMate grammar (`*.tmLanguage.json`) for the Yuzu language from
/// the lexer token definitions in `TokenKind.td`. Keywords and operators come
/// from the `Token` defs' values; numeric/string highlighting reuses the
/// `Regex` defs' patterns verbatim — so highlighting tracks the lexer.
///
/// Unlike the C++ backends this does *not* derive from `CodeGenerator`: that
/// pipes output through clang-format (LLVM C++ style), which would mangle
/// JSON. The grammar is written straight to `os` via `llvm::json`, which
/// pretty-prints on its own.
class TextMateGrammarGenerator {
public:
  explicit TextMateGrammarGenerator(llvm::raw_ostream &os) : os(os) {}

  void run(const llvm::RecordKeeper &records);

private:
  llvm::raw_ostream &os;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_TEXTMATE_GRAMMAR_GENERATOR_H