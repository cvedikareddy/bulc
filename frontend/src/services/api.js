// Mock mode (default) works with no backend. When server.cpp is ready, create
// frontend/.env with:  VITE_USE_MOCK=false   VITE_API_URL=http://localhost:8080
const BASE = import.meta.env.VITE_API_URL || "http://localhost:8080";
const USE_MOCK = import.meta.env.VITE_USE_MOCK !== "false";

export const SAMPLE_SOURCE = `rule HighValueRisky priority 10 {
    when amount > 10000 AND risk_score > 80
    then block_transaction
}

rule SuspiciousCountry priority 5 {
    when country IN ["KP", "IR", "SY"] AND amount >= 500
    then flag_for_review, notify("fraud-team")
}

rule RapidSmallTxns priority 3 within 60s {
    when amount < 50 AND NOT (is_trusted_device == true)
    then raise_alert("velocity")
}`;

// ---- public API ------------------------------------------------------------

// Expected backend response shape for POST /api/compile  {source}:
// { diagnostics, tokens, rules, ir:{before,after}, stats, network:{alphas,betas,terminals} }
export async function compileRules(source) {
  if (USE_MOCK) return mockCompile(source);
  const r = await fetch(`${BASE}/api/compile`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ source }),
  });
  return r.json();
}

// Real backend can return {results, fired} for POST /api/evaluate {source, fact}.
export async function evaluateFact(source, network, fact) {
  if (USE_MOCK) return evalNetwork(network, fact);
  const r = await fetch(`${BASE}/api/evaluate`, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ source, fact }),
  });
  return r.json();
}

// ---- mock compiler ---------------------------------------------------------

const KEYWORDS = new Set(["rule", "priority", "when", "then", "within", "and", "or", "not", "in"]);

function tokenize(src) {
  const re = /#[^\n]*|"[^"]*"|\d+(?:\.\d+)?[smh]?|\w+|>=|<=|==|!=|[<>{}\[\](),]/g;
  return (src.match(re) || [])
    .filter((t) => !t.startsWith("#"))
    .map((t) => {
      let type = "symbol";
      if (t.startsWith('"')) type = "string";
      else if (/^\d/.test(t)) type = "number";
      else if (KEYWORDS.has(t.toLowerCase())) type = "keyword";
      else if (/^\w+$/.test(t)) type = "identifier";
      return { type, lexeme: t };
    });
}

function parseRules(src) {
  const re = /rule\s+(\w+)(?:\s+priority\s+(\d+))?(?:\s+within\s+(\w+))?\s*\{\s*when\s+([\s\S]*?)\s+then\s+([\s\S]*?)\s*\}/gi;
  const rules = [];
  let m;
  while ((m = re.exec(src))) {
    rules.push({
      name: m[1],
      priority: Number(m[2] || 0),
      window: m[3] || null,
      condition: m[4].replace(/\s+/g, " ").trim(),
      actions: m[5].replace(/\s+/g, " ").trim(),
    });
  }
  return rules;
}

const SAMPLE_NETWORK = {
  alphas: [
    { id: 0, label: "amount > 10000", field: "amount", op: ">", values: [10000], usedBy: ["HighValueRisky"] },
    { id: 1, label: "risk_score > 80", field: "risk_score", op: ">", values: [80], usedBy: ["HighValueRisky"] },
    { id: 2, label: 'country IN ["KP","IR","SY"]', field: "country", op: "IN", values: ["KP", "IR", "SY"], usedBy: ["SuspiciousCountry"] },
    { id: 3, label: "amount >= 500", field: "amount", op: ">=", values: [500], usedBy: ["SuspiciousCountry"] },
    { id: 4, label: "amount < 50", field: "amount", op: "<", values: [50], usedBy: ["RapidSmallTxns"] },
    { id: 5, label: "is_trusted_device == true", field: "is_trusted_device", op: "==", values: [true], usedBy: ["RapidSmallTxns"] },
  ],
  betas: [
    { id: 0, kind: "AND", left: "A0", right: "A1" },
    { id: 1, kind: "AND", left: "A2", right: "A3" },
    { id: 2, kind: "NOT", left: "A5", right: null },
    { id: 3, kind: "AND", left: "A4", right: "B2" },
  ],
  terminals: [
    { ruleName: "HighValueRisky", priority: 10, root: "B0", actions: ["block_transaction"] },
    { ruleName: "SuspiciousCountry", priority: 5, root: "B1", actions: ["flag_for_review", 'notify("fraud-team")'] },
    { ruleName: "RapidSmallTxns", priority: 3, root: "B3", actions: ['raise_alert("velocity")'] },
  ],
};

function mockCompile(source) {
  const rules = parseRules(source);
  const diagnostics = rules.length ? [] : [{ severity: "error", message: "no rules found", line: 1, col: 1 }];
  const text = rules.map((r) => ({ rule: r.name, text: r.condition }));
  return new Promise((res) =>
    setTimeout(
      () =>
        res({
          diagnostics,
          tokens: tokenize(source),
          rules,
          ir: { before: text, after: text },
          stats: { nodesBefore: 10, nodesAfter: 10, doubleNotsRemoved: 0, duplicatesRemoved: 0, orsMerged: 0 },
          network: SAMPLE_NETWORK, // mock: always the sample fraud network
        }),
      250
    )
  );
}

// ---- mock evaluator (mirrors Network::evaluate in C++) ---------------------

function alphaTest(a, fact) {
  const v = fact[a.field];
  if (v === undefined) return false;
  if (a.op === "IN") return a.values.includes(v);
  const x = a.values[0];
  switch (a.op) {
    case ">": return typeof v === "number" && v > x;
    case ">=": return typeof v === "number" && v >= x;
    case "<": return typeof v === "number" && v < x;
    case "<=": return typeof v === "number" && v <= x;
    case "==": return v === x;
    case "!=": return v !== x;
    default: return false;
  }
}

export function evalNetwork(network, fact) {
  const results = {};
  network.alphas.forEach((a) => (results["A" + a.id] = alphaTest(a, fact)));
  network.betas.forEach((b) => {
    const l = results[b.left];
    const r = b.right ? results[b.right] : null;
    results["B" + b.id] = b.kind === "AND" ? l && r : b.kind === "OR" ? l || r : !l;
  });
  const fired = network.terminals
    .filter((t) => results[t.root])
    .sort((a, b) => b.priority - a.priority);
  return { results, fired };
}