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

// Punctuation
let IsPunctuation = 1 in {
  def LeftParen  : Token<["("]>;
  def RightParen : Token<[")"]>;
}

// Literals
let IsLiteral = 1 in {
  // Boolean
  def BooleanLiteral    : Token<["true", "false"]>;

  // Numerics
  // Sign is a separate `Plus` / `Minus` token handled by the parser; the
  // lexer never folds a leading `+` / `-` into the literal. Underscores
  // are visual separators only — they must come after the first digit
  // but their inner placement is otherwise unrestricted; the AST drops
  // them when parsing the value.

  // Digit followed by digits-or-underscores: `0`, `42`, `1_000_000`.
  def IntegerLiteral    : Regex<["[0-9][0-9_]*"]>;

  // Decimal forms (`3.14`, `2.`, `.5`) and pure-integer forms with an
  // exponent (`1e10`, `1e-5`). The exponent sign (`e+` / `e-`) IS part of
  // the float — `1e-5` is one token, not three.
  def FloatLiteral      : Regex<["([0-9][0-9_]*\\.[0-9_]*|\\.[0-9][0-9_]*)([eE][+-]?[0-9][0-9_]*)?|[0-9][0-9_]*[eE][+-]?[0-9][0-9_]*"]>;

  // `0x` / `0X` followed by hex digits and underscores.
  def HexLiteral        : Regex<["0[xX][0-9a-fA-F][0-9a-fA-F_]*"]>;

  // `0b` / `0B` followed by binary digits and underscores.
  def BinaryLiteral     : Regex<["0[bB][01][01_]*"]>;

  // String
  // Matches a standard double-quoted string, supporting basic escape sequences like \", \\, \n.
  def StringLiteral     : Regex<["\"([^\"\\\\]|\\\\.)*\""]>;

  // Matches a raw string prefixed by 'r', ignoring all internal escape characters.
  def RawStringLiteral  : Regex<["r\"[^\"]*\""]>;
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
