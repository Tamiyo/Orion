#include "yuzu/tools/TableGen/AstNodeGenerator.h"
#include "yuzu/tools/TableGen/SyntaxKindGenerator.h"
#include "yuzu/tools/TableGen/TokenKindGenerator.h"

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/InitLLVM.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#define DEBUG_TYPE "yuzu-tblgen"

namespace {
enum ActionType {
  GenExprNodeDecls,
  GenSyntaxKindDecls,
  GenTokenKindDecls,
};
} // namespace

static llvm::cl::opt<ActionType> action(
    llvm::cl::desc("Action to perform:"),
    llvm::cl::values(clEnumValN(GenExprNodeDecls, "gen-expr-node-decls",
                                "Generate Expression AstNode declarations")),
    llvm::cl::values(clEnumValN(GenSyntaxKindDecls, "gen-syntax-kind-decls",
                                "Generate SyntaxKind declarations")),
    llvm::cl::values(clEnumValN(GenTokenKindDecls, "gen-token-kind-decls",
                                "Generate TokenKind declarations")));

static bool YuzuTableGenMain(llvm::raw_ostream &os,
                             const llvm::RecordKeeper &records) {
  switch (action) {
  case GenExprNodeDecls: {
    const std::string basename = "Expr";

    const std::set<std::string> projectIncludes = {
        "yuzu/Ast/Ast.h",
        "yuzu/Ast/SyntaxKind.h",
        "yuzu/Syntax/Syntax.h",
    };

    const std::set<std::string> externalIncludes = {};

    const std::set<std::string> systemIncludes = {
        "memory",
        "optional",
        "variant",
        "utility",
    };

    yuzu_tools::AstNodeGenerator(basename, records, projectIncludes,
                                 externalIncludes, systemIncludes)
        .run(os);
    break;
  }

  case GenSyntaxKindDecls: {
    const std::string basename = "SyntaxKind";
    const std::set<std::string> projectIncludes = {};
    const std::set<std::string> externalIncludes = {};
    const std::set<std::string> systemIncludes = {"cstdint"};
    yuzu_tools::SyntaxKindGenerator(basename, records, projectIncludes,
                                    externalIncludes, systemIncludes)
        .run(os);
    break;
  }

  case GenTokenKindDecls: {
    const std::string basename = "TokenKind";
    const std::set<std::string> projectIncludes = {};
    const std::set<std::string> externalIncludes = {};
    const std::set<std::string> systemIncludes = {"cstdint"};
    yuzu_tools::TokenKindGenerator(basename, records, projectIncludes,
                                   externalIncludes, systemIncludes)
        .run(os);
    break;
  }
  }

  return false;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);

  llvm::cl::ParseCommandLineOptions(argc, argv, "Yuzu TableGen");
  return TableGenMain(argv[0], &YuzuTableGenMain);
}
