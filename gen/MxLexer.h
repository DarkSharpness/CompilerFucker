
// Generated from MxLexer.g4 by ANTLR 4.13.0

#pragma once


#include "antlr4-runtime.h"




class  MxLexer : public antlr4::Lexer {
public:
  enum {
    Comment_Multi = 1, Comment_Single = 2, Cstring = 3, NewLine = 4, Blank = 5, 
    BasicTypes = 6, This = 7, Null = 8, True = 9, False = 10, New = 11, 
    Class = 12, Else_if = 13, If = 14, Else = 15, For = 16, While = 17, 
    Break = 18, Continue = 19, Return = 20, Increment = 21, Decrement = 22, 
    Logic_Not = 23, Bit_Inv = 24, Add = 25, Sub = 26, Dot = 27, Mul = 28, 
    Div = 29, Mod = 30, Shift_Right = 31, Shift_Left_ = 32, Cmp_le = 33, 
    Cmp_ge = 34, Cmp_lt = 35, Cmp_gt = 36, Cmp_eq = 37, Cmp_ne = 38, Bit_And = 39, 
    Bit_Xor = 40, Bit_Or_ = 41, Logic_And = 42, Logic_Or_ = 43, Assign = 44, 
    Quest = 45, Colon = 46, Paren_Left_ = 47, Paren_Right = 48, Brack_Left_ = 49, 
    Brack_Right = 50, Brace_Left_ = 51, Brace_Right = 52, Comma = 53, Semi_Colon = 54, 
    Number = 55, Identifier = 56
  };

  enum {
    COMMENTS = 2
  };

  explicit MxLexer(antlr4::CharStream *input);

  ~MxLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

