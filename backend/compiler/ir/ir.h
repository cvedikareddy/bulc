// ir.h - Stage 5: a simplified rule form that is easy to optimize and compile.
#pragma once
#include <memory>
#include <string>
#include <vector>

#include "../parser/ast.h"

namespace bulc {

enum class IRKind { Test, And, Or, Not };

struct IRNode {
    IRKind kind;

    // Test nodes only: `field op value(s)` or `field op otherField`
    std::string field;
    CompareOp op = CompareOp::Eq;
    std::vector<Value> values;      // one value, or many for IN
    bool rhsIsField = false;
    std::string rhsField;

    // And/Or use lhs+rhs, Not uses lhs only
    std::unique_ptr<IRNode> lhs, rhs;

    explicit IRNode(IRKind k) : kind(k) {}

    std::string toString() const;   // readable form
    std::string key() const;        // canonical form (AND/OR operands sorted)
};
using IRPtr = std::unique_ptr<IRNode>;

struct IRRule {
    std::string name;
    int priority = 0;
    double windowMs = 0;
    IRPtr cond;
    std::vector<Action> actions;
};

struct IRProgram {
    std::vector<IRRule> rules;
    std::string toString() const;
};

int countNodes(const IRNode* n);

class IRGenerator {
public:
    IRProgram generate(const Program& program);   // run semantic check first

private:
    IRPtr lower(const Expr* e);
};

}  // namespace bulc