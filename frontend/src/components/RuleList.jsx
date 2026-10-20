export default function RuleList({ rules = [] }) {
  return (
    <div className="card">
      <h3>Rules <span className="muted">({rules.length})</span></h3>
      {rules.length === 0 && <p className="muted">No rules compiled yet.</p>}
      {rules.map((r) => (
        <div key={r.name} className="rule">
          <div className="rule-top">
            <strong>{r.name}</strong>
            <span className="pill">priority {r.priority}</span>
            {r.window && <span className="pill alt">within {r.window}</span>}
          </div>
          <div className="mono small"><span className="muted">when</span> {r.condition}</div>
          <div className="mono small"><span className="muted">then</span> {r.actions}</div>
        </div>
      ))}
    </div>
  );
}