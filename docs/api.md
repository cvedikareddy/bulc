# HTTP API (planned)

> **Status: not implemented.** `backend/api/` does not exist yet. This page records the contract that the frontend (`frontend/src/services/api.js`) already expects, so the C++ server can be written to match it. Default server address: `http://localhost:8080`.

The frontend switches from its built-in mock to the real server by creating `frontend/.env`:

```
VITE_USE_MOCK=false
VITE_API_URL=http://localhost:8080
```

## POST /api/compile

Runs the whole compiler pipeline on rule text.

Request:

```json
{ "source": "rule A { when amount > 100 then block_transaction }" }
```

Response (shape expected by the frontend):

```json
{
  "diagnostics": [ { "severity": "error", "message": "...", "line": 1, "col": 5 } ],
  "tokens":      [ { "type": "identifier", "lexeme": "amount" } ],
  "rules":       [ { "name": "A", "priority": 0, "window": null, "condition": "amount > 100", "actions": "block_transaction" } ],
  "ir":          { "before": [ { "rule": "A", "text": "amount > 100" } ],
                   "after":  [ { "rule": "A", "text": "amount > 100" } ] },
  "stats":       { "nodesBefore": 1, "nodesAfter": 1, "doubleNotsRemoved": 0, "duplicatesRemoved": 0, "orsMerged": 0 },
  "network": {
    "alphas":    [ { "id": 0, "label": "amount > 100", "field": "amount", "op": ">", "values": [100], "usedBy": ["A"] } ],
    "betas":     [ { "id": 0, "kind": "AND", "left": "A0", "right": "A1" } ],
    "terminals": [ { "ruleName": "A", "priority": 0, "root": "A0", "actions": ["block_transaction"] } ]
  }
}
```

Where the data comes from in C++:

| JSON field | Source |
|------------|--------|
| `diagnostics` | `SemanticChecker::check` (`Diagnostic`: severity, message, `loc`) and `LexError` / `ParseError` (message, line, col) |
| `tokens` | `Lexer::tokenize` |
| `rules` | the `Program` AST |
| `ir.before` / `ir.after` | `IRProgram::toString` before and after `Optimizer::run` |
| `stats` | `OptStats` returned by `Optimizer::run` |
| `network` | `Network` (`alphas`, `betas`, `terminals`); `Network::describe()` is a text version of the same data |

If there are errors, return `diagnostics` and omit the later fields.

## POST /api/evaluate

Runs one fact through the compiled network.

Request:

```json
{ "source": "rule A { ... }", "fact": { "amount": 250, "country": "KP" } }
```

Response:

```json
{
  "results": { "A0": true, "A1": false, "B0": false },
  "fired":   [ { "ruleName": "A", "priority": 9, "root": "A0", "actions": ["flag_for_review"] } ]
}
```

`results` holds the true/false answer of every alpha (`A<n>`) and beta (`B<n>`) node, which the frontend uses to colour the graph. `fired` lists the rules that fire, highest priority first (`Network::evaluate`).

## To build

1. Add a small HTTP server in `backend/api/` (for example with a single-header library such as cpp-httplib) plus a JSON writer.
2. Implement the two routes above on top of the compiler headers (see the C++ example in [../backend/README.md](../backend/README.md)).
3. Enable CORS so the Vite dev server (port 5173) can call it.
4. Add API tests, then turn off mock mode in the frontend.
