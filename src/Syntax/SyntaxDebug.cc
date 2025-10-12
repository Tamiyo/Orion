#include "Syntax/SyntaxDebug.h"

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"
#include "Syntax/SyntaxIterator.h"

#include <codecvt>
#include <iomanip>
#include <iostream>
#include <locale>

namespace yuzu::syntax {

static std::string toUtf8(const std::u32string_view &str) {
  std::wstring_convert<std::codecvt_utf8<char32_t>, char32_t> converter;
  return converter.to_bytes(str.data(), str.data() + str.size());
}

static std::string escapeJson(const std::string &str) {
  std::stringstream ss;
  for (char c : str) {
    switch (c) {
    case '"':
      ss << "\\\"";
      break;
    case '\\':
      ss << "\\\\";
      break;
    case '\b':
      ss << "\\b";
      break;
    case '\f':
      ss << "\\f";
      break;
    case '\n':
      ss << "\\n";
      break;
    case '\r':
      ss << "\\r";
      break;
    case '\t':
      ss << "\\t";
      break;
    default:
      if (c >= 0 && c < 0x20) {
        ss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << (int)c;
      } else {
        ss << c;
      }
    }
  }
  return ss.str();
}

static std::string getIndent(int level) { return std::string(level * 2, ' '); }

std::string toJson(const GreenToken &token, int indent) {
  std::stringstream ss;
  ss << "{\n";
  ss << getIndent(indent + 1) << "\"type\": \"GreenToken\",\n";
  ss << getIndent(indent + 1)
     << "\"kind\": " << static_cast<int>(token.getKind()) << ",\n";
  ss << getIndent(indent + 1) << "\"source\": \""
     << escapeJson(toUtf8(token.getSource())) << "\",\n";
  ss << getIndent(indent + 1) << "\"width\": " << token.getWidth() << "\n";
  ss << getIndent(indent) << "}";
  return ss.str();
}

std::string toJson(const GreenNode &node, int indent) {
  std::stringstream ss;
  ss << "{\n";
  ss << getIndent(indent + 1) << "\"type\": \"GreenNode\",\n";
  ss << getIndent(indent + 1)
     << "\"kind\": " << static_cast<int>(node.getKind()) << ",\n";
  ss << getIndent(indent + 1) << "\"width\": " << node.getWidth() << ",\n";
  ss << getIndent(indent + 1) << "\"numChildren\": " << node.getNumChildren()
     << ",\n";
  ss << getIndent(indent + 1) << "\"children\": [";

  auto children = node.getChildren();
  bool first = true;
  for (const auto &child : children) {
    if (!first)
      ss << ",";
    ss << "\n" << getIndent(indent + 2);
    ss << toJson(child, indent + 2);
    first = false;
  }

  if (node.getNumChildren() > 0) {
    ss << "\n" << getIndent(indent + 1);
  }
  ss << "]\n";
  ss << getIndent(indent) << "}";
  return ss.str();
}

std::string toJson(const GreenElement &element, int indent) {
  if (element.isNode()) {
    return toJson(element.getNode(), indent);
  } else if (element.isToken()) {
    return toJson(element.getToken(), indent);
  } else {
    return "null";
  }
}

std::string toJson(const SyntaxToken &token, int indent) {
  std::stringstream ss;
  ss << "{\n";
  ss << getIndent(indent + 1) << "\"type\": \"SyntaxToken\",\n";
  ss << getIndent(indent + 1) << "\"offset\": " << token.getOffset() << ",\n";
  ss << getIndent(indent + 1)
     << "\"kind\": " << static_cast<int>(token.getKind()) << ",\n";
  ss << getIndent(indent + 1) << "\"source\": \""
     << escapeJson(toUtf8(token.getGreen().getSource())) << "\",\n";
  ss << getIndent(indent + 1)
     << "\"parent\": " << (token.getParent() ? "true" : "false") << "\n";
  ss << getIndent(indent) << "}";
  return ss.str();
}

std::string toJson(const SyntaxNode &node, int indent) {
  std::stringstream ss;
  ss << "{\n";
  ss << getIndent(indent + 1) << "\"type\": \"SyntaxNode\",\n";
  ss << getIndent(indent + 1) << "\"offset\": " << node.getOffset() << ",\n";
  ss << getIndent(indent + 1)
     << "\"kind\": " << static_cast<int>(node.getKind()) << ",\n";
  ss << getIndent(indent + 1)
     << "\"parent\": " << (node.getParent() ? "true" : "false") << ",\n";
  ss << getIndent(indent + 1) << "\"childrenWithTokens\": [";

  auto childrenWithTokens = node.getChildrenWithTokens();
  bool first = true;
  for (const auto &child : childrenWithTokens) {
    if (!first)
      ss << ",";
    ss << "\n" << getIndent(indent + 2);
    ss << toJson(child, indent + 2);
    first = false;
  }

  ss << "\n" << getIndent(indent + 1) << "]\n";
  ss << getIndent(indent) << "}";
  return ss.str();
}

std::string toJson(const SyntaxElement &element, int indent) {
  // Directly emit the variant content instead of wrapping it
  if (element.getIfNode()) {
    return toJson(element.getNode(), indent);
  } else if (element.getIfToken()) {
    return toJson(element.getToken(), indent);
  } else {
    return "null";
  }
}

void printJson(const GreenNode &node) {
  std::cout << toJson(node) << std::endl;
}

void printJson(const GreenToken &token) {
  std::cout << toJson(token) << std::endl;
}

void printJson(const GreenElement &element) {
  std::cout << toJson(element) << std::endl;
}

void printJson(const SyntaxNode &node) {
  std::cout << toJson(node) << std::endl;
}

void printJson(const SyntaxToken &token) {
  std::cout << toJson(token) << std::endl;
}

void printJson(const SyntaxElement &element) {
  std::cout << toJson(element) << std::endl;
}

} // namespace yuzu::syntax
