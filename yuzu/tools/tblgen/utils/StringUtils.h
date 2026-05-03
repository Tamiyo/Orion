#ifndef YUZU_TOOLS_TBLGEN_UTILS_STRING_UTILS_H
#define YUZU_TOOLS_TBLGEN_UTILS_STRING_UTILS_H

#include <llvm/ADT/StringRef.h>

#include <cctype>
#include <string>

namespace yuzu::tools {

/// `lhs` → `Lhs`. Used to form accessor names from dag-bound `$names`
/// (`$lhs` → `getLhs`).
inline std::string capitalize(llvm::StringRef name) {
  if (name.empty())
    return std::string();
  std::string out;
  out.push_back(
      static_cast<char>(std::toupper(static_cast<unsigned char>(name[0]))));
  out.append(name.drop_front(1));
  return out;
}

} // namespace yuzu::tools

#endif // YUZU_TOOLS_TBLGEN_UTILS_STRING_UTILS_H
