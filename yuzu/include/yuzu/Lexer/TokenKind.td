include "yuzu/DSL/TokenBase.td"

def Lex : LexGrammar {
  let Namespace = "yuzu::lexer";
}

// Symbols
let IsSymbol = 1 in {
  // Arith
  def Plus        : Token<["+"]>;
  def Minus       : Token<["-"]>;
  def Star        : Token<["*"]>;
  def Pow         : Token<["**"]>;
  def Slash       : Token<["/"]>;

  // Comparison
  def Eq          : Token<["="]>;
  def EqEq        : Token<["=="]>;
  def Neq         : Token<["!="]>;
  def Lt          : Token<["<"]>;
  def Lte         : Token<["<="]>;
  def Gt          : Token<[">"]>;
  def Gte         : Token<[">="]>;

  // Bits
  def ShiftLeft   : Token<["<<"]>;
  def ShiftRight  : Token<[">>"]>;

  // Function-type / return arrow: `(int) -> bool`.
  def Arrow       : Token<["->"]>;
}

// Punctuation
let IsPunctuation = 1 in {
  def LeftParen    : Token<["("]>;
  def RightParen   : Token<[")"]>;
  def LeftBracket  : Token<["["]>;
  def RightBracket : Token<["]"]>;
  def LeftCurly    : Token<["{"]>;
  def RightCurly   : Token<["}"]>;
  def Comma        : Token<[","]>;
  def Colon        : Token<[":"]>;
}

// Literals
let IsLiteral = 1 in {
  // Boolean
  def BooleanLiteral    : Token<["true", "false"]> {
    let Display = "boolean literal";
  }

  // Numerics
  // Sign is a separate `Plus` / `Minus` token handled by the parser; the
  // lexer never folds a leading `+` / `-` into the literal. Underscores
  // are visual separators only — they must come after the first digit
  // but their inner placement is otherwise unrestricted; the AST drops
  // them when parsing the value.

  // Digit followed by digits-or-underscores: `0`, `42`, `1_000_000`.
  def IntegerLiteral    : Regex<["[0-9][0-9_]*"]> {
    let Display = "integer literal";
  }

  // Decimal forms (`3.14`, `2.`, `.5`) and pure-integer forms with an
  // exponent (`1e10`, `1e-5`). The exponent sign (`e+` / `e-`) IS part of
  // the float — `1e-5` is one token, not three.
  def FloatLiteral      : Regex<["([0-9][0-9_]*\\.[0-9_]*|\\.[0-9][0-9_]*)([eE][+-]?[0-9][0-9_]*)?|[0-9][0-9_]*[eE][+-]?[0-9][0-9_]*"]> {
    let Display = "float literal";
  }

  // `0x` / `0X` followed by hex digits and underscores.
  def HexLiteral        : Regex<["0[xX][0-9a-fA-F][0-9a-fA-F_]*"]> {
    let Display = "hex literal";
  }

  // `0b` / `0B` followed by binary digits and underscores.
  def BinaryLiteral     : Regex<["0[bB][01][01_]*"]> {
    let Display = "binary literal";
  }

  // String
  // Matches a standard double-quoted string, supporting basic escape sequences like \", \\, \n.
  def StringLiteral     : Regex<["\"([^\"\\\\]|\\\\.)*\""]> {
    let Display = "string literal";
  }

  // Matches a raw string prefixed by 'r', ignoring all internal escape characters.
  def RawStringLiteral  : Regex<["r\"[^\"]*\""]> {
    let Display = "raw string literal";
  }
}

// Trivia
let IsTrivia = 1 in {
  def Comment : Regex<[""]> { let Display = "comment"; }
  def Newline : Regex<[""]> { let Display = "newline"; }
  def Space   : Regex<[""]> { let Display = "whitespace"; }
}

// Keywords
let IsKeyword = 1 in {
  def AndKw : Token<["and"]>;
  def FnKw : Token<["fn"]>;
  def InKw : Token<["in"]>;
  def LetKw : Token<["let"]>;
  def MutKw : Token<["mut"]>;
  def NotKw : Token<["not"]>;
  def OrKw : Token<["or"]>;
  def ReturnKw : Token<["return"]>;
}

// Misc
def Identifier : Regex<["[a-zA-Z_][a-zA-Z0-9_]*"]> { let Display = "identifier"; }
