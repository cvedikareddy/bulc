// ast.h - Stage 3 data model: the Abstract Syntax Tree produced by the parser.
//
// WHY: The token stream is flat; the AST captures the *structure* of a rule
// (AND/OR nesting, comparisons, actions). Every later stage (semantic check,
// IR generation) walks this tree. Header-only on purpose: it is pure data.
//
//   amount > 10000 AND risk_score > 80   becomes
//
//            AND
//           /  |
//      amount   risk_score
//      > 10000  > 80
#pragma once

#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace bulc {

struct SourceLoc {
    int line = 1;
    int col = 1;
};

// A literal value in the DSL: number, string or boolean.
using Value = std::variant<double, std::string, bool>;

inline std::string valueToString(const Value& v) {
    if (auto d = std::get_if<double>(&v)) {
        std::string s = std::to_string(*d);
        // trim trailing zeros: 10000.000000 -> 10000
        s.erase(s.find_last_not_of('0') + 1);
        if (!s.empty() && s.back() == '.') s.pop_back();
        return s;
    }
    if (auto str = std::get_if<std::string>(&v)) return "\"" + *str + "\"";
    return std::get<bool>(v) ? "true" : "false";
}

enum class ExprKind { Identifier, Literal, List, Compare, Logical, Not };
enum class CompareOp { Gt, Ge, Lt, Le, Eq, Ne, In };
enum class LogicalOp { And, Or };

inline const char* toString(CompareOp op) {
    switch (op) {
        case CompareOp::Gt: return ">";  case CompareOp::Ge: return ">=";
        case CompareOp::Lt: return "<";  case CompareOp::Le: return "<=";
        case CompareOp::Eq: return "=="; case CompareOp::Ne: return "!=";
        case CompareOp::In: return "IN";
    }
    return "?";
}

// ---- Expression nodes ------------------------------------------------------

struct Expr {
    ExprKind kind;
    SourceLoc loc;
    explicit Expr(ExprKind k, SourceLoc l) : kind(k), loc(l) {}
    virtual ~Expr() = default;
    virtual std::string toString() const = 0;   // used for debugging and the UI
};
using ExprPtr = std::unique_ptr<Expr>;

// A field reference such as `amount` (a property of the incoming event/fact).
struct IdentifierExpr : Expr {
    std::string name;
    IdentifierExpr(std::string n, SourceLoc l) : Expr(ExprKind::Identifier, l), name(std::move(n)) {}
    std::string toString() const override { return name; }
};

struct LiteralExpr : Expr {
    Value value;
    LiteralExpr(Value v, SourceLoc l) : Expr(ExprKind::Literal, l), value(std::move(v)) {}
    std::string toString() const override { return valueToString(value); }
};

// ["KP", "IR"] - only valid as the right-hand side of IN.
struct ListExpr : Expr {
    std::vector<Value> items;
    explicit ListExpr(SourceLoc l) : Expr(ExprKind::List, l) {}
    std::string toString() const override {
        std::string s = "[";
        for (size_t i = 0; i < items.size(); ++i) {
            if (i) s += ", ";
            s += valueToString(items[i]);
        }
        return s + "]";
    }
};

// amount > 10000
struct CompareExpr : Expr {
    CompareOp op;
    ExprPtr lhs, rhs;
    CompareExpr(CompareOp o, ExprPtr l, ExprPtr r, SourceLoc loc)
        : Expr(ExprKind::Compare, loc), op(o), lhs(std::move(l)), rhs(std::move(r)) {}
    std::string toString() const override {
        return lhs->toString() + " " + bulc::toString(op) + " " + rhs->toString();
    }
};

// a AND b   /   a OR b
struct LogicalExpr : Expr {
    LogicalOp op;
    ExprPtr lhs, rhs;
    LogicalExpr(LogicalOp o, ExprPtr l, ExprPtr r, SourceLoc loc)
        : Expr(ExprKind::Logical, loc), op(o), lhs(std::move(l)), rhs(std::move(r)) {}
    std::string toString() const override {
        return std::string(op == LogicalOp::And ? "AND" : "OR") + "( " +
               lhs->toString() + ", " + rhs->toString() + " )";
    }
};

// NOT a
struct NotExpr : Expr {
    ExprPtr operand;
    NotExpr(ExprPtr e, SourceLoc l) : Expr(ExprKind::Not, l), operand(std::move(e)) {}
    std::string toString() const override { return "NOT( " + operand->toString() + " )"; }
};

// ---- Rule-level nodes ------------------------------------------------------

// then block_transaction   /   then notify("fraud-team")
struct Action {
    std::string name;
    std::vector<Value> args;
    SourceLoc loc;
};

struct RuleDecl {
    std::string name;
    int priority = 0;          // higher fires first (used by the runtime agenda)
    double windowMs = 0;       // 0 = no time window; otherwise `within <duration>`
    ExprPtr condition;         // the `when` expression
    std::vector<Action> actions;
    SourceLoc loc;
};

// A whole .rule file.
struct Program {
    std::vector<RuleDecl> rules;
};

}  // namespace bulc