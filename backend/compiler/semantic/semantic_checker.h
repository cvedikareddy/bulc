// semantic_checker.h - Stage 4: validates the AST before IR generation.
#pragma once
#include <string>
#include <unordered_map>
#include <vector>

#include "../parser/ast.h"

namespace bulc {

enum class Severity { Error, Warning };

struct Diagnostic {
    Severity severity;
    std::string message;
    SourceLoc loc;
};

enum class FieldType { Number, String, Bool };

class SemanticChecker {
public:
    // Walks the whole program and returns all problems found.
    std::vector<Diagnostic> check(const Program& program);

    static bool hasErrors(const std::vector<Diagnostic>& diags);
    static std::string format(const Diagnostic& d);   // "3:5: error: ..."

    // Field -> type learned from usage (useful later for the IR / UI).
    const std::unordered_map<std::string, FieldType>& fieldTypes() const {
        return fieldTypes_;
    }

private:
    std::vector<Diagnostic> diags_;
    std::unordered_map<std::string, FieldType> fieldTypes_;

    void error(const std::string& msg, SourceLoc loc);
    void warn(const std::string& msg, SourceLoc loc);

    void checkRule(const RuleDecl& rule);
    void checkCondition(const Expr* e);
    void checkCompare(const CompareExpr& c);
    void checkActions(const RuleDecl& rule);
    void noteFieldType(const std::string& field, FieldType t, SourceLoc loc);
};

}  // namespace bulc