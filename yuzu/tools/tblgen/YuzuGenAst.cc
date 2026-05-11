#include "AstNodeGenerator.h"
#include "HirBuilderGenerator.h"
#include "HirKindGenerator.h"
#include "HirNodeGenerator.h"
#include "SyntaxKindGenerator.h"
#include "TokenKindGenerator.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/Record.h>

namespace {
enum ActionType {
  GenAstNodeDecls,
  GenHirBuilderDecls,
  GenHirKindDecls,
  GenHirNodeDecls,
  GenSyntaxKindDecls,
  GenTokenKindDecls,
};
} // namespace

static llvm::cl::opt<ActionType> action(
    llvm::cl::desc("Action to perform:"),
    llvm::cl::values(clEnumValN(GenAstNodeDecls, "gen-ast-node-decls",
                                "Generate AST node and variant declarations")),
    llvm::cl::values(clEnumValN(GenHirBuilderDecls, "gen-hir-builder-decls",
                                "Generate HirBuilder declarations")),
    llvm::cl::values(clEnumValN(GenHirKindDecls, "gen-hir-kind-decls",
                                "Generate HirKind declarations")),
    llvm::cl::values(clEnumValN(GenHirNodeDecls, "gen-hir-node-decls",
                                "Generate HIR node and variant declarations")),
    llvm::cl::values(clEnumValN(GenSyntaxKindDecls, "gen-syntax-kind-decls",
                                "Generate SyntaxKind declarations")),
    llvm::cl::values(clEnumValN(GenTokenKindDecls, "gen-token-kind-decls",
                                "Generate TokenKind declarations")));

static bool YuzuTableGenMain(llvm::raw_ostream &os,
                             const llvm::RecordKeeper &records) {
  switch (action) {
  case GenAstNodeDecls:
    yuzu::tools::AstNodeGenerator(os).run(records);
    break;
  case GenHirBuilderDecls:
    yuzu::tools::HirBuilderGenerator(os).run(records);
    break;
  case GenHirKindDecls:
    yuzu::tools::HirKindGenerator(os).run(records);
    break;
  case GenHirNodeDecls:
    yuzu::tools::HirNodeGenerator(os).run(records);
    break;
  case GenSyntaxKindDecls:
    yuzu::tools::SyntaxKindGenerator(os).run(records);
    break;
  case GenTokenKindDecls:
    yuzu::tools::TokenKindGenerator(os).run(records);
    break;
  }

  return false;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);

  llvm::cl::ParseCommandLineOptions(argc, argv, "Yuzu TableGen");
  return TableGenMain(argv[0], &YuzuTableGenMain);
}
