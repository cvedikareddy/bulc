// parser.cpp - recursive-descent implementation of the grammar in parser.h
#include "parser.h"

namespace bulc {

// ---- token helpers ---------------------------------------------------------

bool Parser::match(TokenType t) {
    if (check(t)) { pos_++; return true; }
    return false;
}

const Token& Parser::advance() {
    if (!check(TokenType::EndOfFile)) pos_++;
    return previous();
}

void Parser::fail(const std::string& msg) const {
    throw ParseError(msg, peek().line, peek().col);
}

const Token& Parser::expect(TokenType t, const std::string& what) {
    if (!check(t))
        fail("expected " + what + " but found " + tokenTypeName(peek().type));
    return advance();
}

// ---- entry points ----------------------------------------------------------

Program Parser::parseSource(const std::string& source) {
    Lexer lexer(source);
    Parser parser(lexer.tokenize());
    return parser.parseProgram();
}

Program Parser::parseProgram() {
    Program prog;
    while (!check(TokenType::EndOfFile)) prog.rules.push_back(parseRule());
    return prog;
}

// ---- rules and actions -----------------------------------------------------

RuleDecl Parser::parseRule() {
    RuleDecl rule;
    const Token& kw = expect(TokenType::Rule, "'rule'");
    rule.loc = {kw.line, kw.col};
    rule.name = expect(TokenType::Identifier, "rule name").lexeme;

    // Optional modifiers, in any order.
    while (true) {
        if (match(TokenType::Priority)) {
            rule.priority = static_cast<int>(expect(TokenType::Number, "priority number").number);
        } else if (match(TokenType::Within)) {
            rule.windowMs = expect(TokenType::Duration, "duration such as 60s").number;
        } else {
            break;
        }
    }

    expect(TokenType::LBrace, "'{'");
    expect(TokenType::When, "'when'");
    rule.condition = parseExpr();
    expect(TokenType::Then, "'then'");
    rule.actions.push_back(parseAction());
    while (match(TokenType::Comma)) rule.actions.push_back(parseAction());
    expect(TokenType::RBrace, "'}'");
    return rule;
}

Action Parser::parseAction() {
    const Token& name = expect(TokenType::Identifier, "action name");
    Action a;
    a.name = name.lexeme;
    a.loc = {name.line, name.col};
    if (match(TokenType::LParen)) {
        a.args.push_back(parseLiteralValue());
        while (match(TokenType::Comma)) a.args.push_back(parseLiteralValue());
        expect(TokenType::RParen, "')'");
    }
    return a;
}

// ---- expressions (one function per precedence level) -----------------------

ExprPtr Parser::parseExpr() { return parseOr(); }

ExprPtr Parser::parseOr() {
    ExprPtr left = parseAnd();
    while (match(TokenType::Or)) {
        SourceLoc loc{previous().line, previous().col};
        left = std::make_unique<LogicalExpr>(LogicalOp::Or, std::move(left), parseAnd(), loc);
    }
    return left;
}

ExprPtr Parser::parseAnd() {
    ExprPtr left = parseUnary();
    while (match(TokenType::And)) {
        SourceLoc loc{previous().line, previous().col};
        left = std::make_unique<LogicalExpr>(LogicalOp::And, std::move(left), parseUnary(), loc);
    }
    return left;
}

ExprPtr Parser::parseUnary() {
    if (match(TokenType::Not)) {
        SourceLoc loc{previous().line, previous().col};
        return std::make_unique<NotExpr>(parseUnary(), loc);
    }
    return parsePrimary();
}

ExprPtr Parser::parsePrimary() {
    if (match(TokenType::LParen)) {
        ExprPtr inner = parseExpr();
        expect(TokenType::RParen, "')'");
        return inner;
    }
    return parseComparison();
}

ExprPtr Parser::parseComparison() {
    ExprPtr lhs = parseOperand();
    SourceLoc loc{peek().line, peek().col};

    if (match(TokenType::In)) {
        expect(TokenType::LBracket, "'[' after IN");
        auto list = std::make_unique<ListExpr>(loc);
        list->items.push_back(parseLiteralValue());
        while (match(TokenType::Comma)) list->items.push_back(parseLiteralValue());
        expect(TokenType::RBracket, "']'");
        return std::make_unique<CompareExpr>(CompareOp::In, std::move(lhs), std::move(list), loc);
    }

    CompareOp op;
    switch (peek().type) {
        case TokenType::Gt: op = CompareOp::Gt; break;
        case TokenType::Ge: op = CompareOp::Ge; break;
        case TokenType::Lt: op = CompareOp::Lt; break;
        case TokenType::Le: op = CompareOp::Le; break;
        case TokenType::Eq: op = CompareOp::Eq; break;
        case TokenType::Ne: op = CompareOp::Ne; break;
        default: fail("expected a comparison operator (>, >=, <, <=, ==, !=, IN)");
    }
    advance();
    return std::make_unique<CompareExpr>(op, std::move(lhs), parseOperand(), loc);
}

ExprPtr Parser::parseOperand() {
    SourceLoc loc{peek().line, peek().col};
    if (check(TokenType::Identifier))
        return std::make_unique<IdentifierExpr>(advance().lexeme, loc);
    return std::make_unique<LiteralExpr>(parseLiteralValue(), loc);
}

Value Parser::parseLiteralValue() {
    if (check(TokenType::Number))   return advance().number;
    if (check(TokenType::String))   return advance().lexeme;
    if (match(TokenType::True))     return true;
    if (match(TokenType::False))    return false;
    fail("expected a number, string, true or false but found " +
         std::string(tokenTypeName(peek().type)));
}

}  // namespace bulc