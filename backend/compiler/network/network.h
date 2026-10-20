// network.h - the compiled rule network plus a simple evaluator.
#pragma once
#include <algorithm>
#include <string>
#include <vector>

#include "alpha_node.h"
#include "beta_node.h"

namespace bulc {

struct TerminalNode {
    std::string ruleName;
    int priority = 0;
    double windowMs = 0;      // stored only; the runtime will enforce it later
    std::vector<Action> actions;
    NodeRef root;             // the node whose result decides if the rule fires
};

class Network {
public:
    std::vector<AlphaNode> alphas;
    std::vector<BetaNode> betas;
    std::vector<TerminalNode> terminals;

    // Returns the rules that fire for this fact, highest priority first.
    std::vector<const TerminalNode*> evaluate(const Fact& fact) const {
        std::vector<signed char> am(alphas.size(), -1), bm(betas.size(), -1);
        std::vector<const TerminalNode*> fired;
        for (const auto& t : terminals)
            if (eval(t.root, fact, am, bm)) fired.push_back(&t);
        std::stable_sort(fired.begin(), fired.end(),
                         [](const TerminalNode* a, const TerminalNode* b) {
                             return a->priority > b->priority;
                         });
        return fired;
    }

    // Text dump (later the API can send this to NetworkGraph.jsx).
    std::string describe() const {
        std::string s = "  Alpha nodes:\n";
        for (const auto& a : alphas) {
            s += "    A" + std::to_string(a.id) + ": " + a.label + "   [";
            for (size_t i = 0; i < a.usedBy.size(); ++i) {
                if (i) s += ", ";
                s += a.usedBy[i];
            }
            s += "]\n";
        }
        s += "  Beta nodes:\n";
        for (const auto& b : betas) {
            const char* k = b.kind == BetaKind::And ? "AND" : b.kind == BetaKind::Or ? "OR" : "NOT";
            s += "    B" + std::to_string(b.id) + ": " + k + "(" + ref(b.left);
            if (b.kind != BetaKind::Not) s += ", " + ref(b.right);
            s += ")\n";
        }
        s += "  Terminals:\n";
        for (const auto& t : terminals) {
            s += "    " + t.ruleName + " (prio " + std::to_string(t.priority) + ") <- " +
                 ref(t.root) + " => ";
            for (size_t i = 0; i < t.actions.size(); ++i) {
                if (i) s += ", ";
                s += t.actions[i].name;
            }
            s += "\n";
        }
        return s;
    }

    static std::string ref(NodeRef r) {
        if (r.index < 0) return "-";
        return (r.isAlpha ? "A" : "B") + std::to_string(r.index);
    }

private:
    // Memoized: each node is computed at most once per fact.
    bool eval(NodeRef r, const Fact& f, std::vector<signed char>& am,
              std::vector<signed char>& bm) const {
        if (r.isAlpha) {
            signed char& m = am[r.index];
            if (m < 0) m = alphas[r.index].test(f) ? 1 : 0;
            return m == 1;
        }
        signed char& m = bm[r.index];
        if (m >= 0) return m == 1;
        const BetaNode& b = betas[r.index];
        bool v = false;
        switch (b.kind) {
            case BetaKind::And: v = eval(b.left, f, am, bm) && eval(b.right, f, am, bm); break;
            case BetaKind::Or:  v = eval(b.left, f, am, bm) || eval(b.right, f, am, bm); break;
            case BetaKind::Not: v = !eval(b.left, f, am, bm); break;
        }
        m = v ? 1 : 0;
        return v;
    }
};

}  // namespace bulc