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
  enum Type { Header, Source };

  explicit AstGenerator(const llvm::RecordKeeper &Records,
                        AstGenerator::Type Type,
                        const std::set<std::string> &ProjectIncludes = {},
                        const std::set<std::string> &ExternalIncludes = {},
                        const std::set<std::string> &SystemIncludes = {})
      : Generator(std::move(Records)), Type_(Type),
        ProjectIncludes_(std::move(ProjectIncludes)),
        ExternalIncludes_(std::move(ExternalIncludes)),
        SystemIncludes_(std::move(SystemIncludes)) {}

protected:
  virtual std::string getIncludeGuardName() const = 0;

  void emitOpenIncludeGuards(llvm::raw_ostream &OS) const {
    const std::string GuardName = getIncludeGuardName();
    OS << llvm::formatv("#ifndef {0}\n#define {0}\n\n", GuardName);
  }

  void emitCloseIncludeGuards(llvm::raw_ostream &OS) const {
    OS << llvm::formatv("#endif // {0}\n", getIncludeGuardName());
  }

  void emitIncludes(llvm::raw_ostream &OS) const {
    const auto EmitIncludes = [&OS](const std::set<std::string> &Includes) {
      if (Includes.empty()) {
        return;
      }

      for (const auto &Include : Includes) {
        if (Include.find(".h") != std::string::npos) {
          OS << "#include \"" << Include << "\"\n";
        } else {
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

  virtual void emitClassDefinitions(llvm::raw_ostream &OS) const = 0;

  virtual void emitClassMethods(llvm::raw_ostream &OS,
                                const llvm::Record *Record) const = 0;

  virtual void emitHeader(llvm::raw_ostream &OS) const = 0;
  virtual void emitSource(llvm::raw_ostream &OS) const = 0;

  const std::set<std::string> IgnoredClasses_ = {
      "AstNode",
      "AstMethod",
  };

  const std::set<std::string> IgnoredSuperClasses_ = {
      "AstMethod",
  };

  Type Type_;

private:
  const std::set<std::string> ProjectIncludes_;

  const std::set<std::string> ExternalIncludes_;

  const std::set<std::string> SystemIncludes_ = {"memory", "optional", "string",
                                                 "variant"};
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_AST_GENERATOR_H
