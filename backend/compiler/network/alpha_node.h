// alpha_node.h - one single-field test, shared by every rule that uses it.
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "../parser/ast.h"

namespace bulc {

// An incoming event: field name -> value.
// NOTE: build string values with std::string("IR"), not "IR" (a const char*
// would silently become a bool inside the variant).
using Fact = std::unordered_map<std::string, Value>;

namespace detail {
inline bool compareValues(CompareOp op, const Value& a, const Value& b) {
    switch (op) {
        case CompareOp::Eq: return a == b;
        case CompareOp::Ne: return a != b;
        case CompareOp::Gt: case CompareOp::Ge:
        case CompareOp::Lt: case CompareOp::Le: {
            auto x = std::get_if<double>(&a);
            auto y = std::get_if<double>(&b);
            if (!x || !y) return false;
            if (op == CompareOp::Gt) return *x > *y;
            if (op == CompareOp::Ge) return *x >= *y;
            if (op == CompareOp::Lt) return *x < *y;
            return *x <= *y;
        }
        case CompareOp::In: return false;   // handled in AlphaNode::test
    }
    return false;
}
}  // namespace detail

struct AlphaNode {
    int id = 0;
    std::string label;                  // e.g. "amount > 10000"
    std::string field;
    CompareOp op = CompareOp::Eq;
    std::vector<Value> values;
    bool rhsIsField = false;
    std::string rhsField;
    std::vector<std::string> usedBy;    // rule names (for the UI graph)

    // A missing field makes the test false.
    bool test(const Fact& f) const {
        auto it = f.find(field);
        if (it == f.end()) return false;
        if (op == CompareOp::In) {
            for (const auto& v : values)
                if (it->second == v) return true;
            return false;
        }
        if (rhsIsField) {
            auto jt = f.find(rhsField);
            if (jt == f.end()) return false;
            return detail::compareValues(op, it->second, jt->second);
        }
        return detail::compareValues(op, it->second, values.front());
    }
};

}  // namespace bulc