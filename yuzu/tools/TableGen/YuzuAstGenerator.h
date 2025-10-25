#ifndef YUZU_TOOLS_AST_GENERATOR_H
#define YUZU_TOOLS_AST_GENERATOR_H

#include "TableGen/AstGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

namespace yuzu_tools {
class YuzuAstGenerator final : public AstGenerator {
public:
  explicit YuzuAstGenerator(const llvm::RecordKeeper &Records)
      : AstGenerator(Records) {}

protected:
  void runImpl(llvm::raw_ostream &OS) override;

private:
  void emitOpenIncludeGuards(llvm::raw_ostream &OS);
  void emitCloseIncludeGuards(llvm::raw_ostream &OS);
  void emitIncludes(llvm::raw_ostream &OS);
  void emitClassDefinitions(llvm::raw_ostream &OS);
  void emitClassMethods(llvm::raw_ostream &OS,
                        const llvm::Record *const &Record);
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_AST_GENERATOR_H
