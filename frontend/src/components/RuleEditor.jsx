export default function RuleEditor({ source, setSource, onCompile, loading, diagnostics = [] }) {
  return (
    <div className="card">
      <div className="card-head">
        <h3>Rule editor</h3>
        <button className="btn" onClick={onCompile} disabled={loading}>
          {loading ? "Compiling..." : "Compile"}
        </button>
      </div>
      <textarea
        className="editor"
        value={source}
        onChange={(e) => setSource(e.target.value)}
        spellCheck={false}
      />
      {diagnostics.length > 0 && (
        <ul className="diags">
          {diagnostics.map((d, i) => (
            <li key={i} className={d.severity}>
              {d.line ? `${d.line}:${d.col} ` : ""}{d.severity}: {d.message}
            </li>
          ))}
        </ul>
      )}
    </div>
  );
}