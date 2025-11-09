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
  explicit Generator(const llvm::RecordKeeper &records) : records(records) {}
  Generator() = delete;

  void run(llvm::raw_ostream &os) {
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);

    runImpl(ss);

    os << formatCode(buffer);
  }

protected:
  virtual void runImpl(llvm::raw_ostream &os) const noexcept = 0;

  // Format a string using clang-format, using a temp file to avoid shell
  // escaping issues.
  static std::string formatCode(const std::string &code) noexcept {
    // Create a temporary file
    char tempFile[] = "/tmp/clang_format_XXXXXX";
    int fd = mkstemp(tempFile);
    if (fd == -1)
      return code;

    // Write code to temp file
    write(fd, code.c_str(), code.size());
    close(fd);

    // Format the file in-place
    std::string cmd = "clang-format -i -style=file ";
    cmd += tempFile;
    system(cmd.c_str());

    // Read back the formatted file
    FILE *file = fopen(tempFile, "r");
    if (!file) {
      unlink(tempFile);
      return code;
    }

    std::string result;
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), file))
      result += buffer;

    fclose(file);
    unlink(tempFile); // Delete temp file

    return result.empty() ? code : result;
  }

  const llvm::RecordKeeper &records;
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_GENERATOR_H
