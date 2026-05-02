#ifndef YUZU_TOOLS_GENERATOR_H
#define YUZU_TOOLS_GENERATOR_H

#include "llvm/Support/FormatVariadic.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/TableGen/Main.h"
#include "llvm/TableGen/Record.h"
#include "llvm/TableGen/TableGenBackend.h"

#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>
#include <unistd.h>
#include <utility>

namespace yuzu_tools {
class CppGenerator {
public:
  explicit CppGenerator(const std::string &basename,
                        const llvm::RecordKeeper &records,
                        const std::set<std::string> &projectIncludes = {},
                        const std::set<std::string> &externalIncludes = {},
                        const std::set<std::string> &systemIncludes = {})
      : records(std::move(records)), basename(std::move(basename)),
        projectIncludes(std::move(projectIncludes)),
        externalIncludes(std::move(externalIncludes)),
        systemIncludes(std::move(systemIncludes)) {}

  CppGenerator() = delete;

  void run(llvm::raw_ostream &os) {
    std::string buffer;
    llvm::raw_string_ostream ss(buffer);

    runImpl(ss);

    os << formatCode(buffer);
  }

protected:
  virtual void runImpl(llvm::raw_ostream &os) const = 0;

  [[nodiscard]] virtual std::string getIncludeGuardName() const = 0;

  virtual void emitClassDefinitions(llvm::raw_ostream &os) const = 0;

  virtual void emitHeader(llvm::raw_ostream &os) const = 0;

  void emitOpenIncludeGuards(llvm::raw_ostream &os) const {
    const std::string guardName = getIncludeGuardName();
    os << llvm::formatv("#ifndef {0}\n#define {0}\n\n", guardName);
  }

  void emitCloseIncludeGuards(llvm::raw_ostream &os) const {
    os << llvm::formatv("#endif // {0}\n", getIncludeGuardName());
  }

  void emitIncludes(llvm::raw_ostream &os) const {
    const auto emitIncludes = [&os](const std::set<std::string> &includes) {
      // Don't emit any includes if there are none, this would emit extra
      // whitespace.
      if (includes.empty()) {
        return;
      }

      for (const auto &include : includes) {
        // Local and customer headers files should use the #include "header"
        // syntax.
        if (include.find(".h") != std::string::npos) {
          os << "#include \"" << include << "\"\n";
        }
        // System headers should use the #include <header> syntax.
        else {
          os << "#include <" << include << ">\n";
        }
      }
      os << "\n";
    };

    /// Emit includes using LLVM style.
    emitIncludes(projectIncludes);
    emitIncludes(externalIncludes);
    emitIncludes(systemIncludes);
  }

  // Format a string using clang-format, using a temp file to avoid shell
  // escaping issues.
  static std::string formatCode(const std::string &code) {
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
  const std::string basename;

private:
  const std::set<std::string> projectIncludes;
  const std::set<std::string> externalIncludes;
  const std::set<std::string> systemIncludes;
};
} // namespace yuzu_tools

#endif // YUZU_TOOLS_GENERATOR_H
