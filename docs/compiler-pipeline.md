# Compiler Pipeline

```
rule source -> Lexer -> Parser -> Semantic check -> IR -> Optimizer -> Rete network
```

| # | Stage | Code | Input -> Output |
|---|-------|------|-----------------|
| 1 | Lexer | `compiler/lexer` | text -> tokens (with line/col). Throws `LexError`. |
| 2 | Parser | `compiler/parser` | tokens -> AST (`Program`). Recursive descent, throws `ParseError`. |
| 3 | Semantic check | `compiler/semantic` | AST -> list of diagnostics (errors/warnings) and inferred field types. |
| 4 | IR generation | `compiler/ir` | AST -> `IRProgram`: rules whose conditions are `Test`/`And`/`Or`/`Not` nodes. |
| 5 | Optimizer | `compiler/optimizer` | IR -> simpler IR plus `OptStats`. |
| 6 | Network compile | `compiler/network` | optimized IR -> shared alpha/beta (Rete) network for the runtime. |

Run the semantic check before IR generation. `IRGenerator::generate` assumes a valid program.

## Optimizations performed

1. `NOT NOT x` -> `x`
2. `x AND x` / `x OR x` -> `x` (operands compared by canonical key)
3. `f == a OR f == b` -> `f IN [a, b]` (also merges into an existing `IN`)
4. Rules sorted by priority, highest first (ties keep file order)

Optimization must never change which rules fire. `tests/optimizer_tests` checks this by evaluating
the unoptimized and optimized IR over a generated fact corpus.

## Network sharing

Identical tests (same field, operator, value) share one alpha node, and identical AND/OR combinations share one beta node,
so two rules that both test `amount > 100` evaluate it once.

## Limitations

The optimizer does not fold constants, detect contradictions (`a > 5 AND a < 3`), merge AND-ed ranges or apply De Morgan.
The compiler stops at the network; executing it against facts is the runtime's job.
