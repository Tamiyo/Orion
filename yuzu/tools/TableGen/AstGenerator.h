#ifndef YUZU_TOOLS_AST_GENERATOR_H
#define YUZU_TOOLS_AST_GENERATOR_H

#include "TableGen/Generator.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <set>
#include <string>
#include <utility>

namespace yuzu_tools {
class AstGenerator : public Generator {
public:
  explicit AstGenerator(const llvm::RecordKeeper &records,
                        const std::set<std::string> &projectIncludes = {},
                        const std::set<std::string> &externalIncludes = {},
                        const std::set<std::string> &systemIncludes = {})
      : Generator(std::move(records)),
        projectIncludes(std::move(projectIncludes)),
        externalIncludes(std::move(externalIncludes)),
        systemIncludes(std::move(systemIncludes)) {}

protected:
  virtual std::string getIncludeGuardName() const noexcept = 0;

  virtual void emitClassDefinitions(llvm::raw_ostream &os) const noexcept = 0;

  virtual void emitHeader(llvm::raw_ostream &os) const noexcept = 0;

  void emitOpenIncludeGuards(llvm::raw_ostream &os) const noexcept {
    const std::string guardName = getIncludeGuardName();
    os << llvm::formatv("#ifndef {0}\n#define {0}\n\n", guardName);
  }

  void emitCloseIncludeGuards(llvm::raw_ostream &os) const noexcept {
    os << llvm::formatv("#endif // {0}\n", getIncludeGuardName());
  }

  void emitIncludes(llvm::raw_ostream &os) const noexcept {
    const auto emitIncludes = [&os](const std::set<std::string> &includes) {
      // Don't emit any includes if there are none, this would emit extra
      // whitespace.
      if (includes.empty()) {
        return;
      }

      for (const auto &include : includes) {
        // Local and customer headers files should use the #include "header"
        // syntax.
        if (include.find(".h") != std::string::npos) {
          os << "#include \"" << include << "\"\n";
        }
        // System headers should use the #include <header> syntax.
        else {
          os << "#include <" << include << ">\n";
        }
      }
      os << "\n";
    };

    /// Emit includes using LLVM style.
    emitIncludes(projectIncludes);
    emitIncludes(externalIncludes);
    emitIncludes(systemIncludes);
  }

  [[nodiscard]] bool
  isIgnored(const llvm::Record *const record) const noexcept {
    // Skip anonymous and base classes. These classes serve as abstractions to
    // generate AstNodes, but are not actually AstNodes themselves.
    if (record->isAnonymous() ||
        ignoredClasses.count(record->getName().str())) {
      return true;
    }

    // Skip if any superclass should be ignored.
    for (const llvm::Record *superClass : record->getSuperClasses()) {
      if (ignoredSuperClasses.count(superClass->getName().str())) {
        return true;
      }
    }

    return false;
  }

  const std::set<std::string> ignoredClasses = {
      "AstNode",
      "AstMethod",
  };

  const std::set<std::string> ignoredSuperClasses = {
      "AstMethod",
  };

private:
  const std::set<std::string> projectIncludes;

  const std::set<std::string> externalIncludes;

  const std::set<std::string> systemIncludes = {"memory", "optional", "string",
                                                "variant"};
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_AST_GENERATOR_H
