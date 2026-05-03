include "yuzu/DSL/TokenBase.td"

def Lex : LexGrammar {
  let Namespace = "yuzu::lexer";
}

// Symbols
let IsSymbol = 1 in {
  def Equals : Token<["="]>;
  def Minus  : Token<["-"]>;
  def Plus   : Token<["+"]>;
  def Slash  : Token<["/"]>;
  def Star   : Token<["*"]>;
}

// Literals
let IsLiteral = 1 in {
  def Number : Regex<[""]>;
}

// Trivia
let IsTrivia = 1 in {
  def Comment : Regex<[""]>;
  def Newline : Regex<[""]>;
  def Space   : Regex<[""]>;
}

// Keywords
let IsKeyword = 1 in {
  def LetKw : Token<["let"]>;
  def MutKw : Token<["mut"]>;
}

// Misc
def Ident : Regex<["[a-zA-Z_][a-zA-Z0-9_]*"]>;
