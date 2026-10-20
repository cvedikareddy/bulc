// optimizer.h - Stage 6: cheap clean-ups on the IR.
#pragma once
#include "../ir/ir.h"

namespace bulc {

struct OptStats {
    int doubleNotsRemoved = 0;
    int duplicatesRemoved = 0;
    int orsMerged = 0;
    int nodesBefore = 0;
    int nodesAfter = 0;
};

class Optimizer {
public:
    OptStats run(IRProgram& program);

private:
    OptStats stats_;
    IRPtr simplify(IRPtr n);
    bool tryMergeOr(IRNode& a, const IRNode& b);
};

}  // namespace bulc