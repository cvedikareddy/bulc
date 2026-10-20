#include "semantic_checker.h"

#include <unordered_set>

namespace bulc {

namespace {

FieldType typeOf(const Value& v) {
    switch (v.index()) {
        case 0: return FieldType::Number;
        case 1: return FieldType::String;
        default: return FieldType::Bool;
    }
}

const char* typeName(FieldType t) {
    switch (t) {
        case FieldType::Number: return "number";
        case FieldType::String: return "string";
        case FieldType::Bool:   return "boolean";
    }
    return "?";
}

bool isOrdered(CompareOp op) {
    return op == CompareOp::Gt || op == CompareOp::Ge ||
           op == CompareOp::Lt || op == CompareOp::Le;
}

// Known actions and how many arguments they take. Edit freely.
const std::unordered_map<std::string, size_t>& knownActions() {
    static const std::unordered_map<std::string, size_t> table = {
        {"block_transaction", 0},
        {"flag_for_review", 0},
        {"notify", 1},
        {"raise_alert", 1},
    };
    return table;
}

}  // namespace

// ---- helpers ---------------------------------------------------------------

void SemanticChecker::error(const std::string& msg, SourceLoc loc) {
    diags_.push_back({Severity::Error, msg, loc});
}

void SemanticChecker::warn(const std::string& msg, SourceLoc loc) {
    diags_.push_back({Severity::Warning, msg, loc});
}

bool SemanticChecker::hasErrors(const std::vector<Diagnostic>& diags) {
    for (const auto& d : diags)
        if (d.severity == Severity::Error) return true;
    return false;
}

std::string SemanticChecker::format(const Diagnostic& d) {
    return std::to_string(d.loc.line) + ":" + std::to_string(d.loc.col) + ": " +
           (d.severity == Severity::Error ? "error" : "warning") + ": " + d.message;
}

// ---- entry point -----------------------------------------------------------

std::vector<Diagnostic> SemanticChecker::check(const Program& program) {
    diags_.clear();
    fieldTypes_.clear();

    std::unordered_set<std::string> seen;
    for (const auto& rule : program.rules) {
        if (!seen.insert(rule.name).second)
            error("duplicate rule name '" + rule.name + "'", rule.loc);
        checkRule(rule);
    }
    return diags_;
}

// ---- per-rule checks -------------------------------------------------------

void SemanticChecker::checkRule(const RuleDecl& rule) {
    if (rule.priority < 0)
        warn("rule '" + rule.name + "' has a negative priority", rule.loc);
    if (rule.windowMs < 0)
        error("rule '" + rule.name + "' has a negative time window", rule.loc);

    if (!rule.condition)
        error("rule '" + rule.name + "' has no condition", rule.loc);
    else
        checkCondition(rule.condition.get());

    checkActions(rule);
}

void SemanticChecker::checkActions(const RuleDecl& rule) {
    if (rule.actions.empty()) {
        error("rule '" + rule.name + "' has no actions", rule.loc);
        return;
    }
    for (const auto& act : rule.actions) {
        auto it = knownActions().find(act.name);
        if (it == knownActions().end()) {
            warn("unknown action '" + act.name + "'", act.loc);
            continue;
        }
        if (act.args.size() != it->second) {
            error("action '" + act.name + "' expects " + std::to_string(it->second) +
                      " argument(s) but got " + std::to_string(act.args.size()),
                  act.loc);
        }
    }
}

// ---- expression checks -----------------------------------------------------

void SemanticChecker::checkCondition(const Expr* e) {
    if (!e) return;
    switch (e->kind) {
        case ExprKind::Compare:
            checkCompare(static_cast<const CompareExpr&>(*e));
            break;
        case ExprKind::Logical: {
            auto& l = static_cast<const LogicalExpr&>(*e);
            checkCondition(l.lhs.get());
            checkCondition(l.rhs.get());
            break;
        }
        case ExprKind::Not:
            checkCondition(static_cast<const NotExpr&>(*e).operand.get());
            break;
        case ExprKind::Identifier:
            // A bare field is treated as a boolean flag.
            noteFieldType(static_cast<const IdentifierExpr&>(*e).name,
                          FieldType::Bool, e->loc);
            break;
        case ExprKind::Literal:
        case ExprKind::List:
            error("expression is not a valid condition", e->loc);
            break;
    }
}

void SemanticChecker::checkCompare(const CompareExpr& c) {
    if (!c.lhs || !c.rhs) {
        error("incomplete comparison", c.loc);
        return;
    }
    const Expr& l = *c.lhs;
    const Expr& r = *c.rhs;

    if (l.kind != ExprKind::Identifier) {
        error("left side of '" + std::string(toString(c.op)) + "' must be a field", l.loc);
        return;
    }
    const auto& field = static_cast<const IdentifierExpr&>(l).name;

    // field IN [ ... ]
    if (c.op == CompareOp::In) {
        if (r.kind != ExprKind::List) {
            error("IN requires a list on the right side", r.loc);
            return;
        }
        const auto& list = static_cast<const ListExpr&>(r);
        if (list.items.empty()) {
            error("IN list is empty", r.loc);
            return;
        }
        FieldType first = typeOf(list.items[0]);
        for (const auto& item : list.items) {
            if (typeOf(item) != first) {
                error("IN list mixes different types", r.loc);
                return;
            }
        }
        noteFieldType(field, first, l.loc);
        return;
    }

    if (r.kind == ExprKind::List) {
        error("a list can only be used with IN", r.loc);
        return;
    }

    // field <op> other-field : nothing more to infer
    if (r.kind != ExprKind::Literal) return;

    FieldType t = typeOf(static_cast<const LiteralExpr&>(r).value);
    if (isOrdered(c.op) && t != FieldType::Number) {
        error("'" + std::string(toString(c.op)) + "' needs a number but got " +
                  typeName(t),
              r.loc);
        return;
    }
    noteFieldType(field, t, l.loc);
}

void SemanticChecker::noteFieldType(const std::string& field, FieldType t, SourceLoc loc) {
    auto it = fieldTypes_.find(field);
    if (it == fieldTypes_.end()) {
        fieldTypes_[field] = t;
    } else if (it->second != t) {
        error("field '" + field + "' used as " + typeName(t) +
                  " but earlier used as " + typeName(it->second),
              loc);
    }
}

}  // namespace bulc