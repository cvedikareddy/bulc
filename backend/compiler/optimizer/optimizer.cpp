#include "optimizer.h"

#include <algorithm>
#include <utility>

namespace bulc {

OptStats Optimizer::run(IRProgram& program) {
    stats_ = OptStats{};
    for (auto& r : program.rules) {
        stats_.nodesBefore += countNodes(r.cond.get());
        r.cond = simplify(std::move(r.cond));
        stats_.nodesAfter += countNodes(r.cond.get());
    }
    // Higher priority first (stable keeps file order for ties).
    std::stable_sort(program.rules.begin(), program.rules.end(),
                     [](const IRRule& a, const IRRule& b) { return a.priority > b.priority; });
    return stats_;
}

// f == a OR f == b  ->  f IN [a, b]   (modifies `a` in place)
bool Optimizer::tryMergeOr(IRNode& a, const IRNode& b) {
    auto mergeable = [](const IRNode& n) {
        return n.kind == IRKind::Test && !n.rhsIsField &&
               (n.op == CompareOp::Eq || n.op == CompareOp::In);
    };
    if (!mergeable(a) || !mergeable(b) || a.field != b.field) return false;

    a.op = CompareOp::In;
    for (const auto& v : b.values) {
        bool present = false;
        for (const auto& x : a.values)
            if (valueToString(x) == valueToString(v)) { present = true; break; }
        if (!present) a.values.push_back(v);
    }
    return true;
}

IRPtr Optimizer::simplify(IRPtr n) {
    if (!n) return n;

    if (n->lhs) n->lhs = simplify(std::move(n->lhs));
    if (n->rhs) n->rhs = simplify(std::move(n->rhs));

    switch (n->kind) {
        case IRKind::Not:
            if (n->lhs->kind == IRKind::Not) {
                stats_.doubleNotsRemoved++;
                return std::move(n->lhs->lhs);
            }
            return n;

        case IRKind::And:
        case IRKind::Or:
            if (n->lhs->key() == n->rhs->key()) {
                stats_.duplicatesRemoved++;
                return std::move(n->lhs);
            }
            if (n->kind == IRKind::Or && tryMergeOr(*n->lhs, *n->rhs)) {
                stats_.orsMerged++;
                return std::move(n->lhs);
            }
            return n;

        case IRKind::Test:
            return n;
    }
    return n;
}

}  // namespace bulc