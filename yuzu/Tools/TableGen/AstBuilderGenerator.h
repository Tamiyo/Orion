#ifndef YUZU_TOOLS_AST_BUILDER_GENERATOR_H
#define YUZU_TOOLS_AST_BUILDER_GENERATOR_H

#include "yuzu/Tools/TableGen/AstGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <set>
#include <string>
#include <utility>

namespace yuzu_tools {
class AstBuilderGenerator final : public AstGenerator {
public:
  explicit AstBuilderGenerator(
      const llvm::RecordKeeper &Records, AstGenerator::Type Type,
      const std::set<std::string> &ProjectIncludes = {},
      const std::set<std::string> &ExternalIncludes = {},
      const std::set<std::string> &SystemIncludes = {})
      : AstGenerator(Records, Type, std::move(ProjectIncludes),
                     std::move(ExternalIncludes), std::move(SystemIncludes)) {}

protected:
  void runImpl(llvm::raw_ostream &OS) const override;

private:
  std::string getIncludeGuardName() const override;

  void emitClassDefinitions(llvm::raw_ostream &OS) const override;

  void emitClassMethods(llvm::raw_ostream &OS,
                        const llvm::Record *Record) const override;

  void emitHeader(llvm::raw_ostream &OS) const override;
  void emitSource(llvm::raw_ostream &OS) const override;
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_AST_BUILDER_GENERATOR_H
