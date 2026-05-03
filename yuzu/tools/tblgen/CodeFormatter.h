#ifndef YUZU_TOOLS_TBLGEN_CODE_FORMATTER_H
#define YUZU_TOOLS_TBLGEN_CODE_FORMATTER_H

#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FormatVariadic.h>
#include <llvm/Support/raw_ostream.h>

#include <cstddef>
#include <utility>

namespace yuzu::tools {

/// Helper for emitting indented source. The current indent level is adjusted
/// by entering and leaving `Scope` RAII guards returned from `block()`.
/// `line` writes a plain string at the current indent; `linef` first runs
/// its arguments through `llvm::formatv` for substitution.
class CodeFormatter final {
public:
  /// RAII indent guard. Construction (via `CodeFormatter::block()`) bumps the
  /// formatter's indent level by one step; destruction restores it.
  /// Move-only — the moved-from instance is inert and skips the dedent.
  class [[nodiscard]] Scope {
  public:
    Scope(const Scope &) = delete;
    Scope &operator=(const Scope &) = delete;
    Scope &operator=(Scope &&) = delete;

    Scope(Scope &&other) noexcept : fmt(std::exchange(other.fmt, nullptr)) {}

    ~Scope() {
      if (fmt) {
        --fmt->indentLevel;
      }
    }

  private:
    friend class CodeFormatter;
    explicit Scope(CodeFormatter *fmt) : fmt(fmt) { ++fmt->indentLevel; }

    CodeFormatter *fmt;
  };

  explicit CodeFormatter(llvm::raw_ostream &os, size_t indent)
      : os(os), indent(indent) {}

  /// Write `content` to a single line at the current indent.
  void line(llvm::StringRef content) {
    os.indent(indent * indentLevel);
    os << content << '\n';
  }

  /// Format-string variant of `line`. `fmt` and `args` are forwarded to
  /// `llvm::formatv`, then the resulting string is written as a single line
  /// at the current indent.
  template <class... Args> void linef(const char *fmt, Args &&...args) {
    line(llvm::formatv(fmt, std::forward<Args>(args)...).str());
  }

  /// Open a new indent scope. The returned guard dedents on destruction.
  [[nodiscard]] Scope block() { return Scope(this); }

private:
  llvm::raw_ostream &os;

  size_t indent;
  size_t indentLevel = 0;
};

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_CODE_FORMATTER_H
