// beta_node.h - combines results of earlier nodes (AND / OR / NOT).
#pragma once

namespace bulc {

enum class BetaKind { And, Or, Not };

// Points at an alpha node or a beta node by index.
struct NodeRef {
    bool isAlpha = true;
    int index = -1;
};

struct BetaNode {
    int id = 0;
    BetaKind kind = BetaKind::And;
    NodeRef left;
    NodeRef right;      // unused for Not
};

}  // namespace bulc