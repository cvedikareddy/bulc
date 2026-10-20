// lexer.cpp - hand-written scanner for the BULC rule DSL.
#include "lexer.h"

#include <algorithm>
#include <cctype>
#include <unordered_map>

namespace bulc {

namespace {
// Keywords are case-insensitive: "AND", "and" and "And" are all the same token.
const std::unordered_map<std::string, TokenType>& keywords() {
    static const std::unordered_map<std::string, TokenType> k = {
        {"rule", TokenType::Rule},     {"priority", TokenType::Priority},
        {"when", TokenType::When},     {"then", TokenType::Then},
        {"within", TokenType::Within}, {"and", TokenType::And},
        {"or", TokenType::Or},         {"not", TokenType::Not},
        {"in", TokenType::In},         {"true", TokenType::True},
        {"false", TokenType::False}};
    return k;
}

std::string lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}
}  // namespace

char Lexer::peek(size_t ahead) const {
    return pos_ + ahead < src_.size() ? src_[pos_ + ahead] : '\0';
}

char Lexer::advance() {
    char c = src_[pos_++];
    if (c == '\n') { line_++; col_ = 1; } else { col_++; }
    return c;
}

Token Lexer::make(TokenType t, std::string lexeme, int line, int col) const {
    Token tok;
    tok.type = t;
    tok.lexeme = std::move(lexeme);
    tok.line = line;
    tok.col = col;
    return tok;
}

// Skips spaces, newlines and comments ('#' or '//' until end of line).
void Lexer::skipWhitespaceAndComments() {
    while (!atEnd()) {
        char c = peek();
        if (std::isspace(static_cast<unsigned char>(c))) {
            advance();
        } else if (c == '#' || (c == '/' && peek(1) == '/')) {
            while (!atEnd() && peek() != '\n') advance();
        } else {
            break;
        }
    }
}

// Numbers: 10000, 3.14. A unit suffix makes it a Duration: 60s, 5m, 2h, 500ms.
// Durations are normalised to milliseconds so later stages use a single unit.
Token Lexer::lexNumberOrDuration() {
    int line = line_, col = col_;
    std::string text;
    while (std::isdigit(static_cast<unsigned char>(peek()))) text += advance();
    if (peek() == '.' && std::isdigit(static_cast<unsigned char>(peek(1)))) {
        text += advance();
        while (std::isdigit(static_cast<unsigned char>(peek()))) text += advance();
    }
    double value = std::stod(text);

    std::string unit;
    while (std::isalpha(static_cast<unsigned char>(peek()))) unit += advance();

    if (unit.empty()) {
        Token t = make(TokenType::Number, text, line, col);
        t.number = value;
        return t;
    }
    double factor;
    if (unit == "ms") factor = 1;
    else if (unit == "s") factor = 1000;
    else if (unit == "m") factor = 60 * 1000;
    else if (unit == "h") factor = 60 * 60 * 1000;
    else throw LexError("unknown time unit '" + unit + "'", line, col);

    Token t = make(TokenType::Duration, text + unit, line, col);
    t.number = value * factor;
    return t;
}

// Strings use double quotes: "KP". Basic escapes: \" \\ \n
Token Lexer::lexString() {
    int line = line_, col = col_;
    advance();  // opening quote
    std::string value;
    while (!atEnd() && peek() != '"') {
        char c = advance();
        if (c == '\\' && !atEnd()) {
            char e = advance();
            value += (e == 'n') ? '\n' : e;
        } else {
            value += c;
        }
    }
    if (atEnd()) throw LexError("unterminated string literal", line, col);
    advance();  // closing quote
    return make(TokenType::String, value, line, col);
}

Token Lexer::lexIdentifierOrKeyword() {
    int line = line_, col = col_;
    std::string text;
    while (std::isalnum(static_cast<unsigned char>(peek())) || peek() == '_' ||
           peek() == '.')  // '.' allows field paths such as user.age
        text += advance();
    auto it = keywords().find(lower(text));
    if (it != keywords().end()) return make(it->second, text, line, col);
    return make(TokenType::Identifier, text, line, col);
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> out;
    while (true) {
        skipWhitespaceAndComments();
        if (atEnd()) break;

        int line = line_, col = col_;
        char c = peek();

        if (std::isdigit(static_cast<unsigned char>(c))) { out.push_back(lexNumberOrDuration()); continue; }
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') { out.push_back(lexIdentifierOrKeyword()); continue; }
        if (c == '"') { out.push_back(lexString()); continue; }

        advance();
        switch (c) {
            case '(': out.push_back(make(TokenType::LParen, "(", line, col)); break;
            case ')': out.push_back(make(TokenType::RParen, ")", line, col)); break;
            case '{': out.push_back(make(TokenType::LBrace, "{", line, col)); break;
            case '}': out.push_back(make(TokenType::RBrace, "}", line, col)); break;
            case '[': out.push_back(make(TokenType::LBracket, "[", line, col)); break;
            case ']': out.push_back(make(TokenType::RBracket, "]", line, col)); break;
            case ',': out.push_back(make(TokenType::Comma, ",", line, col)); break;
            case '>':
                if (peek() == '=') { advance(); out.push_back(make(TokenType::Ge, ">=", line, col)); }
                else out.push_back(make(TokenType::Gt, ">", line, col));
                break;
            case '<':
                if (peek() == '=') { advance(); out.push_back(make(TokenType::Le, "<=", line, col)); }
                else out.push_back(make(TokenType::Lt, "<", line, col));
                break;
            case '=':
                if (peek() == '=') { advance(); out.push_back(make(TokenType::Eq, "==", line, col)); }
                else throw LexError("expected '==' (single '=' is not valid)", line, col);
                break;
            case '!':
                if (peek() == '=') { advance(); out.push_back(make(TokenType::Ne, "!=", line, col)); }
                else throw LexError("expected '!='", line, col);
                break;
            default:
                throw LexError(std::string("unexpected character '") + c + "'", line, col);
        }
    }
    out.push_back(make(TokenType::EndOfFile, "", line_, col_));
    return out;
}

const char* tokenTypeName(TokenType t) {
    switch (t) {
        case TokenType::Rule: return "'rule'";       case TokenType::Priority: return "'priority'";
        case TokenType::When: return "'when'";       case TokenType::Then: return "'then'";
        case TokenType::Within: return "'within'";   case TokenType::And: return "'AND'";
        case TokenType::Or: return "'OR'";           case TokenType::Not: return "'NOT'";
        case TokenType::In: return "'IN'";           case TokenType::True: return "'true'";
        case TokenType::False: return "'false'";     case TokenType::Identifier: return "identifier";
        case TokenType::Number: return "number";     case TokenType::String: return "string";
        case TokenType::Duration: return "duration"; case TokenType::Gt: return "'>'";
        case TokenType::Ge: return "'>='";           case TokenType::Lt: return "'<'";
        case TokenType::Le: return "'<='";           case TokenType::Eq: return "'=='";
        case TokenType::Ne: return "'!='";           case TokenType::LParen: return "'('";
        case TokenType::RParen: return "')'";        case TokenType::LBrace: return "'{'";
        case TokenType::RBrace: return "'}'";        case TokenType::LBracket: return "'['";
        case TokenType::RBracket: return "']'";      case TokenType::Comma: return "','";
        case TokenType::EndOfFile: return "end of file";
    }
    return "?";
}

}  // namespace bulc