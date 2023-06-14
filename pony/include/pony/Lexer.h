//===- Lexer.h - Lexer for the Pony language -------------------------------===//

#ifndef PONY_LEXER_H
#define PONY_LEXER_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

namespace pony {

/// Structure definition a location in a file.
struct Location {
  std::shared_ptr<std::string> file; ///< filename.
  int line;                          ///< line number.
  int col;                           ///< column number.
};

// List of Token returned by the lexer.
enum Token : int {
  tok_semicolon = ';',
  tok_parenthese_open = '(',
  tok_parenthese_close = ')',
  tok_bracket_open = '{',
  tok_bracket_close = '}',
  tok_sbracket_open = '[',
  tok_sbracket_close = ']',

  tok_eof = -1,

  tok_return = -2,
  tok_var = -3,
  tok_def = -4,

  tok_identifier = -5,
  tok_number = -6,
};

/// The Lexer is an abstract base class providing all the facilities that the
/// Parser expects. It goes through the stream one token at a time and keeps
/// track of the location in the file for debugging purpose.
/// It relies on a subclass to provide a `readNextLine()` method. The subclass
/// can proceed by reading the next line from the standard input or from a
/// memory mapped file.
class Lexer {
public:
  /// Create a lexer for the given filename. The filename is kept only for
  /// debugging purpose (attaching a location to a Token).
  Lexer(std::string filename)
      : lastLocation(
            {std::make_shared<std::string>(std::move(filename)), 0, 0}) {}
  virtual ~Lexer() = default;

  /// Look at the current token in the stream.
  Token getCurToken() { return curTok; }

  /// Move to the next token in the stream and return it.
  Token getNextToken() { return curTok = getTok(); }

  /// Move to the next token in the stream, asserting on the current token
  /// matching the expectation.
  void consume(Token tok) {
    assert(tok == curTok && "consume Token mismatch expectation");
    getNextToken();
  }

  /// Return the current identifier (prereq: getCurToken() == tok_identifier)
  llvm::StringRef getId() {
    assert(curTok == tok_identifier);
    return identifierStr;
  }

  double getValue() {

    assert(curTok == tok_number);
    return numVal;
  }

  /// Return the location for the beginning of the current token.
  Location getLastLocation() { return lastLocation; }

  // Return the current line in the file.
  int getLine() { return curLineNum; }

  // Return the current column in the file.
  int getCol() { return curCol; }

  //TODO:
  vector<string> tokens;
  bool is_error = false;

private:
  /// Delegate to a derived class fetching the next line. Returns an empty
  /// string to signal end of file (EOF). Lines are expected to always finish
  /// with "\n"
  virtual llvm::StringRef readNextLine() = 0;


  // TODO: Implement function getNextChar().
  
  // Function description: 该函数从curLineBuffer中获取当前行的下一个char，如果已经处理到当前行最后一个char，则通过读取下一行
  //                       来更新curLineBuffer以确保curLineBuffer非空。

  // Hints: 1. 函数实现过程中可能会用到lexer的部分成员变量（如curLineBuffer）；
  //        2. 注意读到文档结尾，读到某一行结尾等特殊情况的处理。一般来说，读到文档结尾应返回EOF，某一行结尾最后一个char为'\n'；
  //        3. 注意行列位置信息的同步更新；
  //        4. 关于llvm::StringRef的部分函数：llvm::StringRef example; example.front(); example.drop_front(); example.empty()。
  int getNextChar() {
    /* 
     *
     *  Write your code here.
     *
     */
    if(curLineBuffer.empty()) return EOF;
    ++curCol;
    auto nextchar = curLineBuffer.front();
    curLineBuffer = curLineBuffer.drop_front();
    if (curLineBuffer.empty())
      curLineBuffer = readNextLine();
    if (nextchar == '\n') {
      ++curLineNum;
      curCol = 0;
    }
    return nextchar;
  }

  ///  Return the next token from standard input.
  Token getTok() {
    // Skip any whitespace.
    while (isspace(lastChar))
      lastChar = Token(getNextChar());

    // Save the current location before reading the token characters.
    lastLocation.line = curLineNum;
    lastLocation.col = curCol;

    // TODO: 补充成员函数getTok()。
    //       1. 能够识别“return”、“def”和“var”三个关键字；
    //       2. 能够识别标识符（函数名，变量名等）：
    //          • 标识符以字母或下划线开头；
    //          • 标识符由字母、数字和下划线组成；
    //          • 按照使用习惯，要求标识符中有数字时，数字须位于标识符末尾；
    //          例如：有效的标识符可以是 a123, b_4, placeholder 等。
    //       3. 在识别每种Token的同时，将其存放在某种数据结构中，以便最终在终端输出
    //
    // Hints: 1. 在实现第1，2点时，可参考getTok()函数中现有的识别数字的方法。
    //        2. 一些有用的函数:  isalpha(); isalnum();
    /* 
     *
     *  Write your code here.
     *
     */
    if (isalpha(lastChar) || lastChar == '_') {//indicator or key word
      std::string varStr;
      bool no_digit = true;
      bool error_indicator = false;

      while (isalnum(lastChar) || lastChar == '_'){
        if (isdigit(lastChar)) {
          no_digit = false;
        }
        if (!no_digit) {
          if (isalpha(lastChar) || lastChar == '_') {
            error_indicator = true;//the digit is not at the end
            is_error = true;
            cout << "Token error (" << curLineNum << ", " << curCol 
            << "): digits should be at the end of an indicator" << endl;
            break;
          }
        }
        varStr += lastChar;
        lastChar = Token(getNextChar());
      }

      if (!error_indicator) {
        if (varStr == "return") {tokens.push_back("return"); return tok_return;}
        if (varStr == "def") {tokens.push_back("def"); return tok_def;}
        if (varStr == "var") {tokens.push_back("var"); return tok_var;}
        identifierStr = varStr;
        tokens.push_back(identifierStr);
        return tok_identifier;
      }
    }

    //TODO: 3. 改进识别数字的方法，使编译器可以识别并在终端报告非法数字，非法表示包括：9.9.9，9..9，.123等。
    bool first_digit = true;
    bool error_digit = false;
    int point_count = 1;
    if (isdigit(lastChar) || lastChar == '.') {//digit
      std::string numStr;
      do {
        //'.' at the begining
        if(first_digit && lastChar == '.'){
          error_digit = true;
          is_error = true;
          cout << "Token error (" << curLineNum << ", " << curCol 
            << "): the point should not be at the begining of a digit" << endl;
        }
        first_digit = false;

        if(lastChar == '.'){
          point_count--;
          if(point_count < 0){
            error_digit = true;//point appears more than once
            is_error = true;
            cout << "Token error (" << curLineNum << ", " << curCol 
            << "): the point appears more than once in a digit" << endl;
          }
        }
        numStr += lastChar;
        lastChar = Token(getNextChar());
      } while (!error_digit && (isdigit(lastChar) || lastChar == '.'));

      numVal = strtod(numStr.c_str(), nullptr);
      if(!error_digit) tokens.push_back(numStr);
      return tok_number;
    }

    if (lastChar == '#') {
      // Comment until end of line.
      do {
        lastChar = Token(getNextChar());
      } while (lastChar != EOF && lastChar != '\n' && lastChar != '\r');

      if (lastChar != EOF)
        return getTok();
    }

    // Check for end of file.  Don't eat the EOF.
    if (lastChar == EOF)
      return tok_eof;

    // Otherwise, just return the character as its ascii value.
    Token thisChar = Token(lastChar);

    switch(lastChar){
      case ';': tokens.push_back(string(1, ';')); break;
      case '(': tokens.push_back(string(1, '(')); break;
      case ')': tokens.push_back(string(1, ')')); break;
      case '[': tokens.push_back(string(1, '[')); break;
      case ']': tokens.push_back(string(1, ']')); break;
      case '{': tokens.push_back(string(1, '{')); break;
      case '}': tokens.push_back(string(1, '}')); break;
    }

    lastChar = Token(getNextChar());
    return thisChar;
  }

  /// The last token read from the input.
  Token curTok = tok_eof;

  /// Location for `curTok`.
  Location lastLocation;

  /// If the current Token is an identifier, this string contains the value.
  std::string identifierStr;

  /// If the current Token is a number, this contains the value.
  double numVal = 0;

  /// The last value returned by getNextChar(). We need to keep it around as we
  /// always need to read ahead one character to decide when to end a token and
  /// we can't put it back in the stream after reading from it.
  public: Token lastChar = Token(' ');
  private:
  /// Keep track of the current line number in the input stream
  int curLineNum = 0;

  /// Keep track of the current column number in the input stream
  int curCol = 0;

  /// Buffer supplied by the derived class on calls to `readNextLine()`
  llvm::StringRef curLineBuffer = "\n";
  
  

};

/// A lexer implementation operating on a buffer in memory.
class LexerBuffer final : public Lexer {
public:
  LexerBuffer(const char *begin, const char *end, std::string filename)
      : Lexer(std::move(filename)), current(begin), end(end) {}

private:
  /// Provide one line at a time to the Lexer, return an empty string when
  /// reaching the end of the buffer.
  llvm::StringRef readNextLine() override {
    auto *begin = current;
    while (current <= end && *current && *current != '\n')
      ++current;
    if (current <= end && *current)
      ++current;
    llvm::StringRef result{begin, static_cast<size_t>(current - begin)};
    return result;
  }
  const char *current, *end;
};
} // namespace pony

#endif // PONY_LEXER_H
