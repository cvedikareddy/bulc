// lexer.h - Stage 2 of the pipeline: turns raw rule text into tokens.
//
// WHY: The parser should never have to deal with whitespace, comments or
// character-level details. The lexer hides all of that and hands over a clean
// stream of typed tokens, each tagged with a line/column for error messages.
#pragma once

#include <stdexcept>
#include <string>
#include <vector>

namespace bulc {

enum class TokenType {
    // Keywords
    Rule, Priority, When, Then, Within, And, Or, Not, In, True, False,
    // Literals / names
    Identifier, Number, String, Duration,   // Duration: 60s, 5m, 2h, 500ms
    // Comparison operators
    Gt, Ge, Lt, Le, Eq, Ne,
    // Punctuation
    LParen, RParen, LBrace, RBrace, LBracket, RBracket, Comma,
    // Sentinel
    EndOfFile
};

struct Token {
    TokenType type = TokenType::EndOfFile;
    std::string lexeme;     // exact source text (for strings: contents without quotes)
    double number = 0.0;    // value for Number tokens; milliseconds for Duration tokens
    int line = 1;
    int col = 1;
};

// Thrown on malformed input (unknown character, unterminated string, ...).
class LexError : public std::runtime_error {
public:
    LexError(const std::string& msg, int line, int col)
        : std::runtime_error("Lex error at " + std::to_string(line) + ":" +
                             std::to_string(col) + ": " + msg),
          line(line), col(col) {}
    int line, col;
};

class Lexer {
public:
    explicit Lexer(std::string source) : src_(std::move(source)) {}

    // Scans the whole input. The last token is always EndOfFile.
    std::vector<Token> tokenize();

private:
    char peek(size_t ahead = 0) const;
    char advance();
    bool atEnd() const { return pos_ >= src_.size(); }
    void skipWhitespaceAndComments();

    Token lexNumberOrDuration();
    Token lexString();
    Token lexIdentifierOrKeyword();
    Token make(TokenType t, std::string lexeme, int line, int col) const;

    std::string src_;
    size_t pos_ = 0;
    int line_ = 1;
    int col_ = 1;
};

const char* tokenTypeName(TokenType t);

}  // namespace bulc