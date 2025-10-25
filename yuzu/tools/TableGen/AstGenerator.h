#ifndef YUZU_TOOLS_GENERATOR_H
#define YUZU_TOOLS_GENERATOR_H

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

namespace yuzu_tools {
class AstGenerator {
public:
  explicit AstGenerator(const llvm::RecordKeeper &Records)
      : Records_(Records) {}

  AstGenerator() = delete;

  void run(llvm::raw_ostream &OS) { runImpl(OS); }

protected:
  virtual void runImpl(llvm::raw_ostream &OS) = 0;

  const llvm::RecordKeeper &Records_;
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_GENERATOR_H
