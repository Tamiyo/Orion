#ifndef YUZU_TOOLS_TBLGEN_CODE_GENERATOR_H
#define YUZU_TOOLS_TBLGEN_CODE_GENERATOR_H

#include "CodeFormatter.h"

#include <llvm/Support/raw_ostream.h>
#include <llvm/TableGen/Record.h>

#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

namespace yuzu::tools {

/// Base for every yuzu-tblgen backend. Owns an intermediate buffer that
/// `fmt` writes into; `run()` invokes the subclass's `generate()` and pipes
/// the buffer through clang-format before writing to the real output.
class CodeGenerator {
public:
  explicit CodeGenerator(llvm::raw_ostream &os, std::size_t indent = 2)
      : os(os), bufferStream(buffer), fmt(bufferStream, indent) {}
  virtual ~CodeGenerator() = default;

  void run(const llvm::RecordKeeper &records) {
    generate(records);
    os << formatCode(buffer);
  }

protected:
  virtual void generate(const llvm::RecordKeeper &records) = 0;

private:
  /// Format `code` via `clang-format -style=file` against a temp file. Falls
  /// back to the unformatted input on any failure (clang-format missing,
  /// temp-file dance broken) — ugliness never blocks the build.
  static std::string formatCode(const std::string &code) {
    char tempFile[] = "/tmp/yuzu_tblgen_XXXXXX";
    int fd = mkstemp(tempFile);
    if (fd == -1)
      return code;

    write(fd, code.c_str(), code.size());
    close(fd);

    std::string cmd = "clang-format -i -style=file ";
    cmd += tempFile;
    if (system(cmd.c_str()) != 0) {
      unlink(tempFile);
      return code;
    }

    FILE *file = fopen(tempFile, "r");
    if (!file) {
      unlink(tempFile);
      return code;
    }

    std::string result;
    char readBuf[4096];
    while (fgets(readBuf, sizeof(readBuf), file)) {
      result += readBuf;
    }
    fclose(file);
    unlink(tempFile);

    return result.empty() ? code : result;
  }

  // Member declaration order matters: `bufferStream` references `buffer`,
  // and `fmt` references `bufferStream`. Initialization runs in this order
  // regardless of the constructor's initializer list.
  llvm::raw_ostream &os;
  std::string buffer;
  llvm::raw_string_ostream bufferStream;

protected:
  CodeFormatter fmt;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_CODE_GENERATOR_H
