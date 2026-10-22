# BULC Rule Language

A rule file (`.rule`) holds any number of rules. `#` starts a comment that runs to the end of the line.

```
rule SuspiciousCountry priority 5 within 60s {
    when country IN ["KP", "IR"] AND amount >= 500
    then flag_for_review, notify("fraud-team")
}
```

## Grammar (lowest to highest precedence)

```
program    := rule*
rule       := 'rule' IDENT ('priority' NUMBER)? ('within' DURATION)?
              '{' 'when' expr 'then' action (',' action)* '}'
expr       := or
or         := and ('OR' and)*
and        := unary ('AND' unary)*
unary      := 'NOT' unary | primary
primary    := '(' expr ')' | comparison
comparison := operand (cmpop operand | 'IN' list)
operand    := IDENT | NUMBER | STRING | 'true' | 'false'
list       := '[' literal (',' literal)* ']'
action     := IDENT ('(' literal (',' literal)* ')')?
```

Keywords (`rule priority when then within AND OR NOT IN true false`) are case-insensitive.

## Tokens

| Kind | Examples |
|------|----------|
| Identifier (field / rule / action name) | `amount`, `risk_score` |
| Number | `10000`, `3.5` |
| String | `"KP"` |
| Duration | `500ms`, `60s`, `5m`, `2h` (stored as milliseconds) |
| Boolean | `true`, `false` |

## Operators

| Operator | Meaning | Operand types |
|----------|---------|---------------|
| `>` `>=` `<` `<=` | ordered comparison | number only |
| `==` `!=` | equality | number, string, boolean |
| `IN [..]` | membership | non-empty list, all items the same type |
| `NOT` | negation (highest logical precedence) | condition |
| `AND` | conjunction | conditions |
| `OR` | disjunction (lowest precedence) | conditions |

The left side of a comparison must be a field. The right side may be a literal or another field (`a > b`).
Use `==` for equality; a single `=` is a lexer error.

## Semantic rules

Errors: duplicate rule name, rule without actions, negative time window, left side not a field, `IN` without a list, empty or mixed-type `IN` list, a list used without `IN`, ordered operator with a non-number, a field used with two different types, wrong argument count for a known action.
Warnings: negative priority, unknown action name.

Known actions: `block_transaction` (0 args), `flag_for_review` (0), `notify(x)` (1), `raise_alert(x)` (1). Edit the table in `semantic_checker.cpp` to add more.

## Examples

See `backend/examples/`: `fraud_rules.rule`, `insurance_rules.rule`, `firewall_rules.rule`. `ctest` verifies that each one parses, passes the semantic check and compiles.

## Known limitations

- No arithmetic (`amount * 2 > x`), no function calls inside conditions, no string escapes or regex.
- Only numbers (double), strings and booleans exist. There are no dates or nulls.
- Field types are inferred from usage within one program; there is no external schema, so a misspelled field name is not detected.
- `within <duration>` is parsed, stored and carried through the IR, but it is not enforced by the compiler. It is for the runtime.
- Behaviour for a fact that lacks a referenced field is not defined by the compiler.
- Actions are a fixed whitelist. Unknown actions only produce a warning.
