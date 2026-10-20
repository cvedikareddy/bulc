// frontend_test.cpp - checks the lexer and parser against the sample rules.
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

#include "../../compiler/parser/parser.h"

using namespace bulc;

static int failures = 0;
#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) { std::cerr << "FAIL line " << __LINE__ << ": " #cond "\n"; failures++; } \
    } while (0)

static void testLexer() {
    auto toks = Lexer("amount > 10000 AND risk_score > 80").tokenize();
    // amount > 10000 AND risk_score > 80 EOF
    CHECK(toks.size() == 8);
    CHECK(toks[0].type == TokenType::Identifier && toks[0].lexeme == "amount");
    CHECK(toks[1].type == TokenType::Gt);
    CHECK(toks[2].type == TokenType::Number && toks[2].number == 10000);
    CHECK(toks[3].type == TokenType::And);
    CHECK(toks.back().type == TokenType::EndOfFile);

    auto d = Lexer("60s 5m").tokenize();
    CHECK(d[0].type == TokenType::Duration && d[0].number == 60000);
    CHECK(d[1].number == 300000);

    bool threw = false;
    try { Lexer("amount = 5").tokenize(); } catch (const LexError&) { threw = true; }
    CHECK(threw);
}

static void testParserPrecedence() {
    // AND binds tighter than OR; NOT binds tighter than AND.
    Program p = Parser::parseSource(
        "rule R { when a > 1 OR b > 2 AND NOT c == 3 then act }");
    CHECK(p.rules.size() == 1);
    CHECK(p.rules[0].condition->toString() ==
          "OR( a > 1, AND( b > 2, NOT( c == 3 ) ) )");
}

static void testPaperExample() {
    Program p = Parser::parseSource(
        "rule R { when amount > 10000 AND risk_score > 80 then block_transaction }");
    CHECK(p.rules[0].condition->toString() == "AND( amount > 10000, risk_score > 80 )");
    CHECK(p.rules[0].actions[0].name == "block_transaction");
}

static void testErrors() {
    bool threw = false;
    try { Parser::parseSource("rule R { when amount > then x }"); }
    catch (const ParseError& e) { threw = true; std::cout << "  (expected) " << e.what() << "\n"; }
    CHECK(threw);
}

static void testExampleFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) { std::cout << "  skipped example file (not found: " << path << ")\n"; return; }
    std::stringstream ss;
    ss << in.rdbuf();
    Program p = Parser::parseSource(ss.str());
    CHECK(p.rules.size() == 3);
    CHECK(p.rules[0].priority == 10);
    CHECK(p.rules[1].actions.size() == 2);
    CHECK(p.rules[2].windowMs == 60000);
    for (auto& r : p.rules)
        std::cout << "  " << r.name << " (prio " << r.priority << "): "
                  << r.condition->toString() << "\n";
}

int main(int argc, char** argv) {
    testLexer();
    testParserPrecedence();
    testPaperExample();
    testErrors();
    testExampleFile(argc > 1 ? argv[1] : "../examples/fraud_rules.rule");
    std::cout << (failures ? "FAILED\n" : "All front-end tests passed\n");
    return failures ? 1 : 0;
}