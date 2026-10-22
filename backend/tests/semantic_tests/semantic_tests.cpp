// semantic_tests.cpp - invalid fields, type mismatches, nesting, bad actions.
#include "../../compiler/parser/parser.h"
#include "../../compiler/semantic/semantic_checker.h"
#include "../test_util.h"
using namespace bulc;

static bool hasError(const std::string& src, const std::string& needle = "") {
    Program p = Parser::parseSource(src);
    SemanticChecker sc;
    for (const auto& d : sc.check(p))
        if (d.severity == Severity::Error &&
            d.message.find(needle) != std::string::npos) return true;
    return false;
}
static bool clean(const std::string& src) {
    Program p = Parser::parseSource(src);
    SemanticChecker sc;
    return !SemanticChecker::hasErrors(sc.check(p));
}
static bool parseFails(const std::string& src) {
    try { Parser::parseSource(src); } catch (const ParseError&) { return true; }
    return false;
}

int main() {
    // valid
    CHECK(clean("rule A { when amount > 1 then block_transaction }"));
    CHECK(clean("rule A { when c IN [\"KP\",\"IR\"] AND amount >= 5 then notify(\"x\") }"));
    CHECK(clean("rule A { when ((a > 1 OR b < 2) AND NOT (c == true)) then flag_for_review }"));
    CHECK(clean("rule A { when a > b then block_transaction }"));   // field vs field

    // type mismatches
    CHECK(hasError("rule A { when a > \"x\" then block_transaction }", "needs a number"));
    CHECK(hasError("rule A { when a > 1 AND a == \"x\" then block_transaction }", "used as"));
    CHECK(hasError("rule A { when a IN [1, \"x\"] then block_transaction }", "mixes"));
    CHECK(hasError("rule A { when a > 1 AND NOT (a == true) then block_transaction }", "used as"));

    // invalid operands / lists
    CHECK(hasError("rule A { when 5 > a then block_transaction }", "must be a field"));
    CHECK(parseFails("rule A { when a IN [] then block_transaction }"));      // empty list
    CHECK(parseFails("rule A { when a == [1,2] then block_transaction }"));    // list without IN
    CHECK(parseFails("rule A { when a IN 5 then block_transaction }"));        // IN without list

    // rules and actions
    CHECK(hasError("rule A { when a > 1 then block_transaction } "
                   "rule A { when b > 1 then block_transaction }", "duplicate"));
    CHECK(hasError("rule A { when a > 1 then notify }", "expects 1"));
    CHECK(hasError("rule A { when a > 1 then block_transaction(1) }", "expects 0"));
    CHECK(clean("rule A { when a > 1 then mystery_action }"));       // warning only

    // malformed syntax (parser level)
    CHECK(parseFails("rule { when a > 1 then x }"));                 // no name
    CHECK(parseFails("rule A { when a > 1 }"));                      // no then
    CHECK(parseFails("rule A { when a > then x }"));                 // missing operand
    CHECK(parseFails("rule A { when (a > 1 then x }"));              // unbalanced (
    CHECK(parseFails("rule A { when a > 1 then x"));                 // missing }
    CHECK(parseFails("rule A { when a > 1 AND then x }"));           // dangling AND

    // deep nesting does not crash
    std::string deep = "a > 1";
    for (int i = 0; i < 200; ++i) deep = "(" + deep + " AND b < 2)";
    CHECK(clean("rule D { when " + deep + " then block_transaction }"));

    TEST_MAIN_END();
}
