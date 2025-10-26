#ifndef YUZU_TOOLS_GENERATOR_H
#define YUZU_TOOLS_GENERATOR_H

#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <unistd.h>

namespace yuzu_tools {
class Generator {
public:
  explicit Generator(const llvm::RecordKeeper &Records) : Records_(Records) {}
  Generator() = delete;

  void run(llvm::raw_ostream &OS) {
    std::string Buffer;
    llvm::raw_string_ostream SS(Buffer);

    runImpl(SS);

    OS << formatCode(Buffer);
  }

protected:
  virtual void runImpl(llvm::raw_ostream &OS) const = 0;

  // Format a string using clang-format
  // Uses a temp file to avoid shell escaping issues
  static std::string formatCode(const std::string &Code) {
    // Create a temporary file
    char tempFile[] = "/tmp/clang_format_XXXXXX";
    int fd = mkstemp(tempFile);
    if (fd == -1) return Code;

    // Write code to temp file
    write(fd, Code.c_str(), Code.size());
    close(fd);

    // Format the file in-place
    std::string cmd = "clang-format -i -style=file ";
    cmd += tempFile;
    system(cmd.c_str());

    // Read back the formatted file
    FILE* file = fopen(tempFile, "r");
    if (!file) {
      unlink(tempFile);
      return Code;
    }

    std::string result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), file))
      result += buffer;

    fclose(file);
    unlink(tempFile);  // Delete temp file

    return result.empty() ? Code : result;
  }

  const llvm::RecordKeeper &Records_;
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_GENERATOR_H
