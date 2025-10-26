#include "yuzu/Tools/TableGen/AstNodeGenerator.h"

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <cstdint>
#include <utility>

namespace yuzu_tools {
std::string AstNodeGenerator::getIncludeGuardName() const {
  const llvm::StringRef Basename =
      llvm::sys::path::stem(Records_.getInputFilename());
  return "YUZU_AST_" + Basename.upper() + "_INC_H";
}

void AstNodeGenerator::emitClassDefinitions(llvm::raw_ostream &OS) const {
  // Forward declare any abstract classes.
  for (const auto &[Name, Record] : Records_.getClasses()) {
    // Skip any classes that should be ignored.
    if (Record->isAnonymous() || IgnoredClasses_.count(Name)) {
      continue;
    }

    // Skip if any superclass should be ignored.
    bool hasIgnoredSuperClass = false;
    for (const llvm::Record *SuperClass : Record->getSuperClasses()) {
      if (IgnoredSuperClasses_.count(SuperClass->getName().str())) {
        hasIgnoredSuperClass = true;
        break;
      }
    }

    if (hasIgnoredSuperClass) {
      continue;
    }

    OS << "class " << Name << ";\n";
  }

  // Emit the class defs.
  for (const auto &[Name, Record] : Records_.getDefs()) {
    // Skip any classes that should be ignored.
    if (Record->isAnonymous() || IgnoredClasses_.count(Name)) {
      continue;
    }

    // Skip if any superclass should be ignored (e.g., AstMethod)
    bool hasIgnoredSuperClass = false;
    for (const llvm::Record *SuperClass : Record->getSuperClasses()) {
      if (IgnoredSuperClasses_.count(SuperClass->getName().str())) {
        hasIgnoredSuperClass = true;
        break;
      }
    }

    if (hasIgnoredSuperClass) {
      continue;
    }

    OS << llvm::formatv(R"(
class {0} final : public AstNode {{
public:
  explicit {0}(syntax::SyntaxNode Node) : AstNode(std::move(Node)) {{}}

  {0}() = delete;
  {0}(const {0} &) = delete;
  {0} &operator=(const {0} &) = delete;

)",
    Name);

    emitClassMethods(OS, Record.get());

    OS << "\n};\n\n";
  }

  // Helper to create comma-separated list of subclasses
  const auto getSubClassList = [](const auto &Subclasses) {
    std::string Result;
    for (size_t i = 0, s = Subclasses.size(); i < s; i++) {
      Result += Subclasses[i]->getName().str();
      if (i != s - 1) {
        Result += ", ";
      }
    }
    return Result;
  };

  // Emit the abstract class defs as implementations of std::variant.
  for (const auto &[Name, Record] : Records_.getClasses()) {
    // Skip any classes that should be ignored.
    if (Record->isAnonymous() || IgnoredClasses_.count(Name)) {
      continue;
    }

    const auto Subclasses = Records_.getAllDerivedDefinitionsIfDefined(Name);
    const std::string SubClassList = getSubClassList(Subclasses);

    OS << llvm::formatv(R"(
class {0} : public std::variant<{1}> {{
  using std::variant<{1}>::variant;

  {0}() = delete;
}};
)",
    Name, SubClassList);
  }
}

void AstNodeGenerator::emitNthAstMethod(llvm::raw_ostream &OS,
                                        const llvm::Record *Record) const {
  const llvm::StringRef MethodName = Record->getValueAsString("MethodName");
  const llvm::StringRef IteratorType = Record->getValueAsString("IteratorType");
  const llvm::StringRef BuilderType = Record->getValueAsString("BuilderType");
  const llvm::StringRef ReturnType = Record->getValueAsString("ReturnType");
  const int64_t Nth = Record->getValueAsInt("Nth");

  std::string IteratorClass;
  std::string IteratorGetter;
  if (IteratorType == "Children") {
    IteratorClass = "syntax::SyntaxChildren";
    IteratorGetter = "getChildren";
  } else if (IteratorType == "ChildrenWithTokens") {
    IteratorClass = "syntax::SyntaxChildrenWithTokens";
    IteratorGetter = "getChildrenWithTokens";
  } else {
    llvm::errs() << "Unknown Iterator class: " << IteratorClass << "\n";
  }

  // Determine the return logic based on BuilderType
  std::string ReturnLogic;
  if (BuilderType == "Expr") {
    ReturnLogic = "  return ExprBuilder::tryFrom(std::move(Child));";
  } else if (BuilderType == "Token") {
    ReturnLogic =
        R"(  if (const syntax::SyntaxToken *Token = Child.getIfToken()) {
    return Token->getGreen().getSource();
  })";
  } else {
    llvm::errs() << "Unknown Builder class: " << BuilderType << "\n";
    ReturnLogic = "  // Unknown builder type";
  }

  OS << llvm::formatv(R"(
[[nodiscard]] {0} {1}() const {{
  const {2} Children = Node_.{3}();

  size_t Nth = 0;
  auto It = Children.begin(), End = Children.end();
  for (; It != End; It++) {{
    if (Nth == {4}) {{
      break;
    }}
    Nth += 1;
  }}

  if (It == End) {{
    return nullptr;
  }}

  const auto Child = *It;
{5}
}}
)",
                      ReturnType, MethodName, IteratorClass, IteratorGetter,
                      Nth, ReturnLogic);
}

void AstNodeGenerator::emitClassMethods(llvm::raw_ostream &OS,
                                        const llvm::Record *Record) const {
  (void)OS;
  (void)Record;
}

void AstNodeGenerator::emitHeader(llvm::raw_ostream &OS) const {
  emitSourceFileHeader("Yuzu AST Node Declarations", OS);

  emitOpenIncludeGuards(OS);
  emitIncludes(OS);

  OS << "namespace yuzu::ast {\n";

  emitClassDefinitions(OS);

  OS << "} // namespace yuzu::ast\n\n";

  emitCloseIncludeGuards(OS);
}

void AstNodeGenerator::emitSource(llvm::raw_ostream &OS) const { (void)OS; }

void AstNodeGenerator::runImpl(llvm::raw_ostream &OS) const {
  if (Type_ == AstGenerator::Type::Source) {
    emitSource(OS);
  } else {
    emitHeader(OS);
  }
}
} // namespace yuzu_tools
