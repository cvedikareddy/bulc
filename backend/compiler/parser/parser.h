// parser.h - Stage 3: recursive-descent parser. Tokens in, AST out.
//
// Grammar (lowest to highest precedence):
//
//   program    := rule*
//   rule       := 'rule' IDENT ('priority' NUMBER)? ('within' DURATION)?
//                 '{' 'when' expr 'then' action (',' action)* '}'
//   expr       := or
//   or         := and ('OR' and)*
//   and        := unary ('AND' unary)*
//   unary      := 'NOT' unary | primary
//   primary    := '(' expr ')' | comparison
//   comparison := operand (cmpop operand | 'IN' list)
//   operand    := IDENT | NUMBER | STRING | 'true' | 'false'
//   list       := '[' literal (',' literal)* ']'
//   action     := IDENT ('(' literal (',' literal)* ')')?
//
// Because each precedence level is its own function, NOT binds tighter than
// AND, which binds tighter than OR - exactly like most programming languages.
#pragma once

#include <stdexcept>
#include <vector>

#include "../lexer/lexer.h"
#include "ast.h"

namespace bulc {

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& msg, int line, int col)
        : std::runtime_error("Parse error at " + std::to_string(line) + ":" +
                             std::to_string(col) + ": " + msg),
          line(line), col(col) {}
    int line, col;
};

class Parser {
public:
    explicit Parser(std::vector<Token> tokens) : toks_(std::move(tokens)) {}

    Program parseProgram();

    // Convenience: lex + parse in one call.
    static Program parseSource(const std::string& source);

private:
    const Token& peek() const { return toks_[pos_]; }
    const Token& previous() const { return toks_[pos_ - 1]; }
    bool check(TokenType t) const { return peek().type == t; }
    bool match(TokenType t);
    const Token& advance();
    const Token& expect(TokenType t, const std::string& what);
    [[noreturn]] void fail(const std::string& msg) const;

    RuleDecl parseRule();
    Action parseAction();
    ExprPtr parseExpr();
    ExprPtr parseOr();
    ExprPtr parseAnd();
    ExprPtr parseUnary();
    ExprPtr parsePrimary();
    ExprPtr parseComparison();
    ExprPtr parseOperand();
    Value parseLiteralValue();

    std::vector<Token> toks_;
    size_t pos_ = 0;
};

}  // namespace bulc