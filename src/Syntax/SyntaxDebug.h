#ifndef SYNTAX_SYNTAXDEBUG_H
#define SYNTAX_SYNTAXDEBUG_H

#include "Syntax/Green/Green.h"
#include "Syntax/Syntax.h"

#include <string>

namespace yuzu::syntax {

std::string toJson(const GreenNode &node, int indent = 0);
std::string toJson(const GreenToken &token, int indent = 0);
std::string toJson(const GreenElement &element, int indent = 0);
std::string toJson(const SyntaxNode &node, int indent = 0);
std::string toJson(const SyntaxToken &token, int indent = 0);
std::string toJson(const SyntaxElement &element, int indent = 0);

void printJson(const GreenNode &node);
void printJson(const GreenToken &token);
void printJson(const GreenElement &element);
void printJson(const SyntaxNode &node);
void printJson(const SyntaxToken &token);
void printJson(const SyntaxElement &element);
} // namespace yuzu::syntax

#endif // SYNTAX_SYNTAXDEBUG_H
