#ifndef YUZU_TOOLS_AST_NODE_GENERATOR_H
#define YUZU_TOOLS_AST_NODE_GENERATOR_H

#include "yuzu/Tools/TableGen/AstGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <set>
#include <string>

namespace yuzu_tools {
class AstNodeGenerator final : public AstGenerator {
public:
  explicit AstNodeGenerator(const llvm::RecordKeeper &Records,
                            AstGenerator::Type Type,
                            const std::set<std::string> &ProjectIncludes = {},
                            const std::set<std::string> &ExternalIncludes = {},
                            const std::set<std::string> &SystemIncludes = {})
      : AstGenerator(Records, Type, std::move(ProjectIncludes),
                     std::move(ExternalIncludes), std::move(SystemIncludes)) {}

protected:
  void runImpl(llvm::raw_ostream &OS) const noexcept override;

private:
  std::string getIncludeGuardName() const noexcept override;

  void emitClassDefinitions(llvm::raw_ostream &OS) const noexcept override;

  void emitNthAstMethod(llvm::raw_ostream &OS,
                        const llvm::Record *Record) const noexcept;

  void emitClassMethods(llvm::raw_ostream &OS,
                        const llvm::Record *Record) const noexcept override;

  void emitHeader(llvm::raw_ostream &OS) const noexcept override;

  void emitSource(llvm::raw_ostream &OS) const noexcept override;
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_AST_NODE_GENERATOR_H
