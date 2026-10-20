import { useState } from "react";

const STAGES = ["Lexer", "Parser", "Semantic", "IR", "Optimizer", "Network"];

export default function CompilationView({ result }) {
  const [stage, setStage] = useState("Lexer");
  if (!result) return <div className="card">Compile something first.</div>;

  const diags = result.diagnostics || [];
  const stats = result.stats || {};

  return (
    <div className="card">
      <div className="subtabs">
        {STAGES.map((s) => (
          <button key={s} className={s === stage ? "tab active" : "tab"} onClick={() => setStage(s)}>
            {s}
          </button>
        ))}
      </div>

      {stage === "Lexer" && (
        <div>
          <p className="muted">{result.tokens?.length || 0} tokens</p>
          <div className="tokens">
            {(result.tokens || []).map((t, i) => (
              <span key={i} className={"tok " + t.type} title={t.type}>{t.lexeme}</span>
            ))}
          </div>
        </div>
      )}

      {stage === "Parser" && (
        <div>
          <p className="muted">Abstract syntax tree: one entry per rule</p>
          {(result.rules || []).map((r) => (
            <div key={r.name} className="rule">
              <strong>{r.name}</strong>
              <div className="mono small">condition: {r.condition}</div>
              <div className="mono small">actions: {r.actions}</div>
            </div>
          ))}
        </div>
      )}

      {stage === "Semantic" && (
        <div>
          {diags.length === 0 ? (
            <p className="ok">No errors or warnings.</p>
          ) : (
            <ul className="diags">
              {diags.map((d, i) => <li key={i} className={d.severity}>{d.severity}: {d.message}</li>)}
            </ul>
          )}
        </div>
      )}

      {stage === "IR" && (
        <div>
          <p className="muted">Flattened conditions before optimization</p>
          {(result.ir?.before || []).map((x) => (
            <div key={x.rule} className="mono small"><strong>{x.rule}</strong>: {x.text}</div>
          ))}
        </div>
      )}

      {stage === "Optimizer" && (
        <div>
          <div className="stats">
            <Stat label="Nodes before" value={stats.nodesBefore} />
            <Stat label="Nodes after" value={stats.nodesAfter} />
            <Stat label="Double NOTs removed" value={stats.doubleNotsRemoved} />
            <Stat label="Duplicates removed" value={stats.duplicatesRemoved} />
            <Stat label="ORs merged to IN" value={stats.orsMerged} />
          </div>
          <p className="muted">Conditions after optimization</p>
          {(result.ir?.after || []).map((x) => (
            <div key={x.rule} className="mono small"><strong>{x.rule}</strong>: {x.text}</div>
          ))}
        </div>
      )}

      {stage === "Network" && (
        <div className="cols">
          <div>
            <h4>Alpha nodes</h4>
            {(result.network?.alphas || []).map((a) => (
              <div key={a.id} className="mono small">A{a.id}: {a.label} <span className="muted">[{a.usedBy.join(", ")}]</span></div>
            ))}
          </div>
          <div>
            <h4>Beta nodes</h4>
            {(result.network?.betas || []).map((b) => (
              <div key={b.id} className="mono small">B{b.id}: {b.kind}({b.left}{b.right ? ", " + b.right : ""})</div>
            ))}
          </div>
          <div>
            <h4>Terminals</h4>
            {(result.network?.terminals || []).map((t) => (
              <div key={t.ruleName} className="mono small">{t.ruleName} (p{t.priority}) &lt;- {t.root}</div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}

export function Stat({ label, value }) {
  return (
    <div className="stat">
      <div className="stat-value">{value ?? "-"}</div>
      <div className="stat-label">{label}</div>
    </div>
  );
}