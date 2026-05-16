#include "yuzu/Syntax/Green/GreenPrinter.h"

#include "yuzu/Syntax/Green/Green.h"
#include "yuzu/Util/ErrorHandling.h"
#include "yuzu/Util/Unicode.h"

#include <llvm/Support/raw_ostream.h>

#include <cstddef>
#include <string>
#include <variant>

namespace yuzu::syntax {

void GreenPrinter::print(const GreenNode &node) { printNode(node, 0, 0); }

void GreenPrinter::print(const GreenToken &token) { printToken(token, 0, 0); }

void GreenPrinter::print(const GreenElement &element) {
  if (const GreenNode *node = element.getIfNode()) {
    printNode(*node, 0, 0);
    return;
  }

  if (const GreenToken *token = element.getIfToken()) {
    printToken(*token, 0, 0);
    return;
  }

  util::yuzu_unreachable();
}

std::string GreenPrinter::printToString(const GreenNode &node) {
  std::string out;
  llvm::raw_string_ostream os(out);
  GreenPrinter(os).print(node);
  return out;
}

std::string GreenPrinter::printToString(const GreenToken &token) {
  std::string out;
  llvm::raw_string_ostream os(out);
  GreenPrinter(os).print(token);
  return out;
}

std::string GreenPrinter::printToString(const GreenElement &element) {
  std::string out;
  llvm::raw_string_ostream os(out);
  GreenPrinter(os).print(element);
  return out;
}

void GreenPrinter::printNode(const GreenNode &node, size_t offset,
                             size_t indent) {
  os.indent(indent * 2);
  os << "Node " << node.getKind() << '@' << offset << ".."
     << (offset + node.getWidth());

  for (const GreenChild &child : node.getChildren()) {
    os << '\n';
    const size_t childOffset = offset + child.relativeOffset;
    if (const auto *childNode = std::get_if<GreenNode>(&child.element)) {
      printNode(*childNode, childOffset, indent + 1);
    } else if (const auto *childToken =
                   std::get_if<GreenToken>(&child.element)) {
      printToken(*childToken, childOffset, indent + 1);
    } else {
      util::yuzu_unreachable();
    }
  }
}

void GreenPrinter::printToken(const GreenToken &token, size_t offset,
                              size_t indent) {
  os.indent(indent * 2);
  os << "Token " << token.getKind() << '@' << offset << ".."
     << (offset + token.getWidth()) << " \"";
  util::writeEscapedQuoted(os, token.getSource());
  os << "\"";
}

} // namespace yuzu::syntax
