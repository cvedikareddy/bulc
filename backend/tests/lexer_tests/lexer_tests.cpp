// lexer_tests.cpp - valid tokens and malformed input for the lexer.
#include "../../compiler/lexer/lexer.h"
#include "../test_util.h"
using namespace bulc;

static bool lexFails(const std::string& s) {
    try { Lexer(s).tokenize(); } catch (const LexError&) { return true; }
    return false;
}

int main() {
    // keywords are case-insensitive
    auto k = Lexer("RULE rule When THEN And or NOT in true FALSE").tokenize();
    CHECK(k[0].type == TokenType::Rule && k[1].type == TokenType::Rule);
    CHECK(k[2].type == TokenType::When && k[3].type == TokenType::Then);
    CHECK(k[4].type == TokenType::And && k[5].type == TokenType::Or);
    CHECK(k[6].type == TokenType::Not && k[7].type == TokenType::In);
    CHECK(k[8].type == TokenType::True && k[9].type == TokenType::False);

    // operators
    auto o = Lexer("> >= < <= == !=").tokenize();
    CHECK(o[0].type == TokenType::Gt && o[1].type == TokenType::Ge);
    CHECK(o[2].type == TokenType::Lt && o[3].type == TokenType::Le);
    CHECK(o[4].type == TokenType::Eq && o[5].type == TokenType::Ne);

    // numbers, durations, strings
    auto n = Lexer("3.5 42 500ms 60s 5m 2h \"KP\"").tokenize();
    CHECK(n[0].type == TokenType::Number && n[0].number == 3.5);
    CHECK(n[1].number == 42);
    CHECK(n[2].type == TokenType::Duration && n[2].number == 500);
    CHECK(n[3].number == 60000 && n[4].number == 300000 && n[5].number == 7200000);
    CHECK(n[6].type == TokenType::String && n[6].lexeme == "KP");

    // comments and positions
    auto c = Lexer("# comment\namount").tokenize();
    CHECK(c[0].type == TokenType::Identifier && c[0].line == 2 && c[0].col == 1);
    CHECK(c.back().type == TokenType::EndOfFile);

    // empty input -> just EOF
    CHECK(Lexer("").tokenize().size() == 1);

    // malformed input
    CHECK(lexFails("amount = 5"));        // single '='
    CHECK(lexFails("\"unterminated"));    // open string
    CHECK(lexFails("amount @ 5"));        // unknown character
    CHECK(lexFails("amount ! 5"));        // lone '!'

    TEST_MAIN_END();
}
