#include "AstNodeGenerator.h"
#include "CodeFormatter.h"
#include "SyntaxKindGenerator.h"
#include "TokenKindGenerator.h"

#include <llvm/Support/CommandLine.h>
#include <llvm/Support/InitLLVM.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Main.h>
#include <llvm/TableGen/Record.h>

#include <utility>

namespace {
enum ActionType {
  GenAstNodeDecls,
  GenSyntaxKindDecls,
  GenTokenKindDecls,
};
} // namespace

static llvm::cl::opt<ActionType> action(
    llvm::cl::desc("Action to perform:"),
    llvm::cl::values(clEnumValN(GenAstNodeDecls, "gen-ast-node-decls",
                                "Generate AST node and variant declarations")),
    llvm::cl::values(clEnumValN(GenSyntaxKindDecls, "gen-syntax-kind-decls",
                                "Generate SyntaxKind declarations")),
    llvm::cl::values(clEnumValN(GenTokenKindDecls, "gen-token-kind-decls",
                                "Generate TokenKind declarations")));

static bool YuzuTableGenMain(llvm::raw_ostream &os,
                             const llvm::RecordKeeper &records) {
  yuzu::tools::CodeFormatter fmt(os, /*indent=*/2);
  switch (action) {
  case GenAstNodeDecls:
    yuzu::tools::AstNodeGenerator(std::move(fmt)).generate(records);
    break;
  case GenSyntaxKindDecls:
    yuzu::tools::SyntaxKindGenerator(std::move(fmt)).generate(records);
    break;
  case GenTokenKindDecls:
    yuzu::tools::TokenKindGenerator(std::move(fmt)).generate(records);
    break;
  }

  return false;
}

int main(int argc, char **argv) {
  llvm::InitLLVM X(argc, argv);

  llvm::cl::ParseCommandLineOptions(argc, argv, "Yuzu TableGen");
  return TableGenMain(argv[0], &YuzuTableGenMain);
}
