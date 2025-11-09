#ifndef YUZU_TOOLS_AST_NODE_GENERATOR_H
#define YUZU_TOOLS_AST_NODE_GENERATOR_H

#include "yuzu/tools/TableGen/AstGenerator.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <set>
#include <string>

namespace yuzu_tools {
class AstNodeGenerator final : public AstGenerator {
public:
  explicit AstNodeGenerator(const llvm::RecordKeeper &records,
                            const std::set<std::string> &projectIncludes = {},
                            const std::set<std::string> &externalIncludes = {},
                            const std::set<std::string> &systemIncludes = {})
      : AstGenerator(records, std::move(projectIncludes),
                     std::move(externalIncludes), std::move(systemIncludes)) {}

protected:
  void runImpl(llvm::raw_ostream &os) const noexcept override;

private:
  std::string getIncludeGuardName() const noexcept override;

  void emitClassDefinitions(llvm::raw_ostream &os) const noexcept override;

  void emitHeader(llvm::raw_ostream &os) const noexcept override;
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_AST_NODE_GENERATOR_H
