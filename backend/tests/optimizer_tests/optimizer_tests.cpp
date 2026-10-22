// optimizer_tests.cpp - optimized and unoptimized IR must give identical
// results over a generated fact corpus.
#include <map>
#include <set>
#include <variant>

#include "../../compiler/ir/ir.h"
#include "../../compiler/optimizer/optimizer.h"
#include "../../compiler/parser/parser.h"
#include "../../compiler/semantic/semantic_checker.h"
#include "../test_util.h"
using namespace bulc;

using Fact = std::map<std::string, Value>;

static bool cmpValues(const Value& a, CompareOp op, const Value& b) {
    if (a.index() != b.index()) return false;
    if (auto x = std::get_if<double>(&a)) {
        double y = std::get<double>(b);
        switch (op) {
            case CompareOp::Gt: return *x > y;  case CompareOp::Ge: return *x >= y;
            case CompareOp::Lt: return *x < y;  case CompareOp::Le: return *x <= y;
            case CompareOp::Eq: return *x == y; case CompareOp::Ne: return *x != y;
            default: return false;
        }
    }
    if (op == CompareOp::Eq) return a == b;
    if (op == CompareOp::Ne) return a != b;
    return false;
}

static bool eval(const IRNode* n, const Fact& f) {
    switch (n->kind) {
        case IRKind::And: return eval(n->lhs.get(), f) && eval(n->rhs.get(), f);
        case IRKind::Or:  return eval(n->lhs.get(), f) || eval(n->rhs.get(), f);
        case IRKind::Not: return !eval(n->lhs.get(), f);
        case IRKind::Test: {
            auto it = f.find(n->field);
            if (it == f.end()) return false;               // missing field -> false
            if (n->op == CompareOp::In) {
                for (const auto& v : n->values) if (cmpValues(it->second, CompareOp::Eq, v)) return true;
                return false;
            }
            if (n->rhsIsField) {
                auto jt = f.find(n->rhsField);
                return jt != f.end() && cmpValues(it->second, n->op, jt->second);
            }
            return !n->values.empty() && cmpValues(it->second, n->op, n->values[0]);
        }
    }
    return false;
}

static std::vector<Fact> corpus() {
    std::vector<Fact> out;
    const double amounts[] = {0, 49, 50, 100, 101, 499, 500, 10000, 10001};
    const double risks[] = {0, 80, 81};
    const char* countries[] = {"KP", "IR", "SY", "US"};
    for (double a : amounts) for (double r : risks) for (auto c : countries)
        for (bool t : {false, true}) {
            out.push_back({{"amount", a}, {"risk_score", r}, {"country", std::string(c)},
                           {"is_trusted_device", t}});
        }
    out.push_back({});                                      // empty fact
    return out;
}

static std::set<std::string> fired(const IRProgram& p, const Fact& f) {
    std::set<std::string> s;
    for (const auto& r : p.rules) if (eval(r.cond.get(), f)) s.insert(r.name);
    return s;
}

static IRProgram build(const std::string& src) {
    Program p = Parser::parseSource(src);
    SemanticChecker sc;
    CHECK(!SemanticChecker::hasErrors(sc.check(p)));
    return IRGenerator().generate(p);
}

static void checkEquivalent(const std::string& src) {
    IRProgram plain = build(src), opt = build(src);
    Optimizer().run(opt);
    for (const auto& f : corpus()) CHECK(fired(plain, f) == fired(opt, f));
}

int main() {
    checkEquivalent(
        "rule A priority 1 { when NOT (NOT (amount > 100)) AND amount > 100 then flag_for_review }");
    checkEquivalent(
        "rule B { when country == \"KP\" OR country == \"IR\" OR country == \"SY\" then flag_for_review }");
    checkEquivalent(
        "rule C { when country IN [\"KP\",\"IR\"] OR country == \"US\" then flag_for_review }");
    checkEquivalent(
        "rule D { when (amount > 10 OR amount > 10) AND NOT (is_trusted_device == true) then block_transaction }");
    checkEquivalent(
        "rule E { when ((amount < 50 OR risk_score > 80) AND country != \"US\") "
        "OR NOT (amount >= 500 AND risk_score <= 80) then block_transaction }");
    checkEquivalent(
        "rule F { when NOT (NOT (NOT (amount > 5))) then block_transaction }");

    // the example rule file's rules
    checkEquivalent(
        "rule HighValueRisky priority 10 { when amount > 10000 AND risk_score > 80 then block_transaction }"
        "rule SuspiciousCountry priority 5 { when country IN [\"KP\",\"IR\",\"SY\"] AND amount >= 500 "
        "then flag_for_review, notify(\"fraud-team\") }"
        "rule RapidSmallTxns priority 3 within 60s { when amount < 50 AND NOT (is_trusted_device == true) "
        "then raise_alert(\"velocity\") }");

    // optimizer actually does something
    IRProgram p = build("rule A { when NOT (NOT (amount > 1)) then block_transaction }");
    OptStats st = Optimizer().run(p);
    CHECK(st.doubleNotsRemoved == 1 && st.nodesAfter < st.nodesBefore);
    IRProgram q = build("rule B { when c == \"A\" OR c == \"B\" then block_transaction }");
    CHECK(Optimizer().run(q).orsMerged == 1);

    // priority ordering
    IRProgram r = build("rule L priority 1 { when a > 1 then block_transaction }"
                        "rule H priority 9 { when b > 1 then block_transaction }");
    Optimizer().run(r);
    CHECK(r.rules[0].name == "H");

    TEST_MAIN_END();
}
