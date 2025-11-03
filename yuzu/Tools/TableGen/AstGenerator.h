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
  explicit AstGenerator(const llvm::RecordKeeper &Records,
                        const std::set<std::string> &ProjectIncludes = {},
                        const std::set<std::string> &ExternalIncludes = {},
                        const std::set<std::string> &SystemIncludes = {})
      : Generator(std::move(Records)),
        ProjectIncludes_(std::move(ProjectIncludes)),
        ExternalIncludes_(std::move(ExternalIncludes)),
        SystemIncludes_(std::move(SystemIncludes)) {}

protected:
  virtual std::string getIncludeGuardName() const noexcept = 0;

  virtual void emitClassDefinitions(llvm::raw_ostream &OS) const noexcept = 0;

  virtual void emitHeader(llvm::raw_ostream &OS) const noexcept = 0;

  void emitOpenIncludeGuards(llvm::raw_ostream &OS) const noexcept {
    const std::string GuardName = getIncludeGuardName();
    OS << llvm::formatv("#ifndef {0}\n#define {0}\n\n", GuardName);
  }

  void emitCloseIncludeGuards(llvm::raw_ostream &OS) const noexcept {
    OS << llvm::formatv("#endif // {0}\n", getIncludeGuardName());
  }

  void emitIncludes(llvm::raw_ostream &OS) const noexcept {
    const auto EmitIncludes = [&OS](const std::set<std::string> &Includes) {
      // Don't emit any includes if there are none, this would emit extra
      // whitespace.
      if (Includes.empty()) {
        return;
      }

      for (const auto &Include : Includes) {
        // Local and customer headers files should use the #include "header"
        // syntax.
        if (Include.find(".h") != std::string::npos) {
          OS << "#include \"" << Include << "\"\n";
        }
        // System headers should use the #include <header> syntax.
        else {
          OS << "#include <" << Include << ">\n";
        }
      }
      OS << "\n";
    };

    /// Emit includes using LLVM style.
    EmitIncludes(ProjectIncludes_);
    EmitIncludes(ExternalIncludes_);
    EmitIncludes(SystemIncludes_);
  }

  [[nodiscard]] bool
  isIgnored(const llvm::Record *const Record) const noexcept {
    // Skip anonymous and base classes. These classes serve as abstractions to
    // generate AstNodes, but are not actually AstNodes themselves.
    if (Record->isAnonymous() ||
        IgnoredClasses_.count(Record->getName().str())) {
      return true;
    }

    // Skip if any superclass should be ignored.
    for (const llvm::Record *SuperClass : Record->getSuperClasses()) {
      if (IgnoredSuperClasses_.count(SuperClass->getName().str())) {
        return true;
      }
    }

    return false;
  }

  const std::set<std::string> IgnoredClasses_ = {
      "AstNode",
      "AstMethod",
  };

  const std::set<std::string> IgnoredSuperClasses_ = {
      "AstMethod",
  };

private:
  const std::set<std::string> ProjectIncludes_;

  const std::set<std::string> ExternalIncludes_;

  const std::set<std::string> SystemIncludes_ = {"memory", "optional", "string",
                                                 "variant"};
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_AST_GENERATOR_H
