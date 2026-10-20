#include "rete_compiler.h"

#include <utility>

namespace bulc {

Network ReteCompiler::compile(const IRProgram& program) {
    net_ = Network{};
    alphaIndex_.clear();
    betaIndex_.clear();

    for (const auto& r : program.rules) {
        TerminalNode t;
        t.ruleName = r.name;
        t.priority = r.priority;
        t.windowMs = r.windowMs;
        t.actions = r.actions;
        t.root = build(r.cond.get(), r.name);
        net_.terminals.push_back(std::move(t));
    }
    return std::move(net_);
}

NodeRef ReteCompiler::build(const IRNode* n, const std::string& ruleName) {
    switch (n->kind) {
        case IRKind::Test:
            return alphaFor(*n, ruleName);
        case IRKind::Not:
            return betaFor(BetaKind::Not, build(n->lhs.get(), ruleName), NodeRef{});
        case IRKind::And:
        case IRKind::Or: {
            NodeRef l = build(n->lhs.get(), ruleName);
            NodeRef r = build(n->rhs.get(), ruleName);
            return betaFor(n->kind == IRKind::And ? BetaKind::And : BetaKind::Or, l, r);
        }
    }
    return NodeRef{};
}

NodeRef ReteCompiler::alphaFor(const IRNode& test, const std::string& ruleName) {
    std::string key = test.key();
    auto it = alphaIndex_.find(key);
    int idx;
    if (it == alphaIndex_.end()) {
        AlphaNode a;
        a.id = (int)net_.alphas.size();
        a.label = test.toString();
        a.field = test.field;
        a.op = test.op;
        a.values = test.values;
        a.rhsIsField = test.rhsIsField;
        a.rhsField = test.rhsField;
        net_.alphas.push_back(std::move(a));
        idx = net_.alphas.back().id;
        alphaIndex_[key] = idx;
    } else {
        idx = it->second;
    }
    auto& users = net_.alphas[idx].usedBy;
    if (users.empty() || users.back() != ruleName) users.push_back(ruleName);
    return NodeRef{true, idx};
}

NodeRef ReteCompiler::betaFor(BetaKind kind, NodeRef l, NodeRef r) {
    std::string a = Network::ref(l), b = Network::ref(r);
    if (kind != BetaKind::Not && b < a) std::swap(a, b);   // AND/OR are commutative
    std::string key = std::to_string((int)kind) + ":" + a + "," + b;

    auto it = betaIndex_.find(key);
    if (it != betaIndex_.end()) return NodeRef{false, it->second};

    BetaNode node;
    node.id = (int)net_.betas.size();
    node.kind = kind;
    node.left = l;
    node.right = r;
    net_.betas.push_back(node);
    betaIndex_[key] = node.id;
    return NodeRef{false, node.id};
}

}  // namespace bulc