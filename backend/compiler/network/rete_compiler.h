// rete_compiler.h - Stage 7: builds the shared network from the optimized IR.
#pragma once
#include <string>
#include <unordered_map>

#include "../ir/ir.h"
#include "network.h"

namespace bulc {

class ReteCompiler {
public:
    Network compile(const IRProgram& program);

private:
    Network net_;
    std::unordered_map<std::string, int> alphaIndex_;   // test key -> alpha id
    std::unordered_map<std::string, int> betaIndex_;    // node key -> beta id

    NodeRef build(const IRNode* n, const std::string& ruleName);
    NodeRef alphaFor(const IRNode& test, const std::string& ruleName);
    NodeRef betaFor(BetaKind kind, NodeRef l, NodeRef r);
};

}  // namespace bulc