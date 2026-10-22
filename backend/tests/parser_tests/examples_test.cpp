// examples_test.cpp - every example .rule file must lex, parse, pass the
// semantic check, generate IR and compile to a network. Usage: examples_test FILE...
#include <fstream>
#include <sstream>

#include "../../compiler/ir/ir.h"
#include "../../compiler/network/rete_compiler.h"
#include "../../compiler/optimizer/optimizer.h"
#include "../../compiler/parser/parser.h"
#include "../../compiler/semantic/semantic_checker.h"
#include "../test_util.h"
using namespace bulc;

int main(int argc, char** argv) {
    CHECK(argc > 1);
    for (int i = 1; i < argc; ++i) {
        std::ifstream in(argv[i]);
        CHECK(in.good());
        std::stringstream ss; ss << in.rdbuf();
        try {
            Program p = Parser::parseSource(ss.str());
            CHECK(!p.rules.empty());
            SemanticChecker sc;
            auto diags = sc.check(p);
            for (const auto& d : diags) std::cerr << argv[i] << ":" << SemanticChecker::format(d) << "\n";
            CHECK(!SemanticChecker::hasErrors(diags));
            IRProgram ir = IRGenerator().generate(p);
            Optimizer().run(ir);
            ReteCompiler().compile(ir);
        } catch (const std::exception& e) {
            std::cerr << argv[i] << ": " << e.what() << "\n";
            g_failures++;
        }
    }
    TEST_MAIN_END();
}
