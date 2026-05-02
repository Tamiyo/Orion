#ifndef YUZU_TOOLS_TOKEN_KIND_GENERATOR_H
#define YUZU_TOOLS_TOKEN_KIND_GENERATOR_H

#include "CppGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <set>
#include <string>

namespace yuzu_tools {
class TokenKindGenerator final : public CppGenerator {
public:
  explicit TokenKindGenerator(
      const std::string &basename, const llvm::RecordKeeper &records,
      const std::set<std::string> &projectIncludes = {},
      const std::set<std::string> &externalIncludes = {},
      const std::set<std::string> &systemIncludes = {})
      : CppGenerator(basename, records, std::move(projectIncludes),
                     std::move(externalIncludes), std::move(systemIncludes)) {}

protected:
  void runImpl(llvm::raw_ostream &os) const override;

private:
  [[nodiscard]] std::string getIncludeGuardName() const override {
    const llvm::StringRef basenameRef(basename);
    return "YUZU_AST_" + basenameRef.upper() + "_INC_H";
  }

  void emitClassDefinitions(llvm::raw_ostream &os) const override;
  void emitInlineMethods(llvm::raw_ostream &os) const;

  void emitHeader(llvm::raw_ostream &os) const override;
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_AST_NODE_GENERATOR_H
