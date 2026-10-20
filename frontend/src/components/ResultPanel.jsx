export default function ResultPanel({ fired }) {
  return (
    <div className="card">
      <h3>Fired rules</h3>
      {!fired && <p className="muted">Nothing run yet.</p>}
      {fired && fired.length === 0 && <p className="muted">No rules fired for this fact.</p>}
      {fired && fired.length > 0 && (
        <table className="table">
          <thead><tr><th>#</th><th>Rule</th><th>Priority</th><th>Actions</th></tr></thead>
          <tbody>
            {fired.map((t, i) => (
              <tr key={t.ruleName}>
                <td>{i + 1}</td>
                <td><strong>{t.ruleName}</strong></td>
                <td>{t.priority}</td>
                <td className="mono small">{t.actions.join(", ")}</td>
              </tr>
            ))}
          </tbody>
        </table>
      )}
    </div>
  );
}