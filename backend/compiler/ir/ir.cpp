#include "ir.h"

#include <stdexcept>
#include <utility>

namespace bulc {

std::string IRNode::toString() const {
    switch (kind) {
        case IRKind::Test: {
            std::string rhsText;
            if (rhsIsField) {
                rhsText = rhsField;
            } else if (op == CompareOp::In) {
                rhsText = "[";
                for (size_t i = 0; i < values.size(); ++i) {
                    if (i) rhsText += ", ";
                    rhsText += valueToString(values[i]);
                }
                rhsText += "]";
            } else {
                rhsText = valueToString(values.front());
            }
            return field + " " + bulc::toString(op) + " " + rhsText;
        }
        case IRKind::And:
            return "AND( " + lhs->toString() + ", " + rhs->toString() + " )";
        case IRKind::Or:
            return "OR( " + lhs->toString() + ", " + rhs->toString() + " )";
        case IRKind::Not:
            return "NOT( " + lhs->toString() + " )";
    }
    return "?";
}

std::string IRNode::key() const {
    switch (kind) {
        case IRKind::Test:
            return toString();
        case IRKind::Not:
            return "NOT(" + lhs->key() + ")";
        case IRKind::And:
        case IRKind::Or: {
            std::string a = lhs->key(), b = rhs->key();
            if (b < a) std::swap(a, b);   // a AND b == b AND a
            return std::string(kind == IRKind::And ? "AND(" : "OR(") + a + ", " + b + ")";
        }
    }
    return "?";
}

int countNodes(const IRNode* n) {
    if (!n) return 0;
    return 1 + countNodes(n->lhs.get()) + countNodes(n->rhs.get());
}

std::string IRProgram::toString() const {
    std::string s;
    for (const auto& r : rules) {
        s += "  rule " + r.name + " (prio " + std::to_string(r.priority);
        if (r.windowMs > 0) s += ", window " + std::to_string((long long)r.windowMs) + "ms";
        s += "): " + r.cond->toString() + "\n";
        s += "      -> ";
        for (size_t i = 0; i < r.actions.size(); ++i) {
            if (i) s += ", ";
            s += r.actions[i].name;
            if (!r.actions[i].args.empty()) {
                s += "(";
                for (size_t j = 0; j < r.actions[i].args.size(); ++j) {
                    if (j) s += ", ";
                    s += valueToString(r.actions[i].args[j]);
                }
                s += ")";
            }
        }
        s += "\n";
    }
    return s;
}

IRProgram IRGenerator::generate(const Program& program) {
    IRProgram out;
    for (const auto& r : program.rules) {
        IRRule ir;
        ir.name = r.name;
        ir.priority = r.priority;
        ir.windowMs = r.windowMs;
        ir.actions = r.actions;
        ir.cond = lower(r.condition.get());
        out.rules.push_back(std::move(ir));
    }
    return out;
}

IRPtr IRGenerator::lower(const Expr* e) {
    if (!e) throw std::runtime_error("IR: missing condition");

    switch (e->kind) {
        case ExprKind::Identifier: {
            // bare field means `field == true`
            auto t = std::make_unique<IRNode>(IRKind::Test);
            t->field = static_cast<const IdentifierExpr&>(*e).name;
            t->op = CompareOp::Eq;
            t->values.push_back(Value(true));
            return t;
        }
        case ExprKind::Compare: {
            auto& c = static_cast<const CompareExpr&>(*e);
            if (c.lhs->kind != ExprKind::Identifier)
                throw std::runtime_error("IR: left side of comparison must be a field");
            auto t = std::make_unique<IRNode>(IRKind::Test);
            t->field = static_cast<const IdentifierExpr&>(*c.lhs).name;
            t->op = c.op;
            switch (c.rhs->kind) {
                case ExprKind::Literal:
                    t->values.push_back(static_cast<const LiteralExpr&>(*c.rhs).value);
                    break;
                case ExprKind::List:
                    t->values = static_cast<const ListExpr&>(*c.rhs).items;
                    break;
                case ExprKind::Identifier:
                    t->rhsIsField = true;
                    t->rhsField = static_cast<const IdentifierExpr&>(*c.rhs).name;
                    break;
                default:
                    throw std::runtime_error("IR: unsupported right side of comparison");
            }
            return t;
        }
        case ExprKind::Logical: {
            auto& l = static_cast<const LogicalExpr&>(*e);
            auto n = std::make_unique<IRNode>(l.op == LogicalOp::And ? IRKind::And : IRKind::Or);
            n->lhs = lower(l.lhs.get());
            n->rhs = lower(l.rhs.get());
            return n;
        }
        case ExprKind::Not: {
            auto n = std::make_unique<IRNode>(IRKind::Not);
            n->lhs = lower(static_cast<const NotExpr&>(*e).operand.get());
            return n;
        }
        default:
            throw std::runtime_error("IR: expression is not a valid condition");
    }
}

}  // namespace bulc