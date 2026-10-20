// Shows every node's true/false result for one fact, in evaluation order.
export default function ExecutionTrace({ network, results }) {
  if (!network || !results) return <div className="card"><h3>Execution trace</h3><p className="muted">Run a fact to see the trace.</p></div>;

  const rows = [
    ...network.alphas.map((a) => ({ key: "A" + a.id, text: a.label })),
    ...network.betas.map((b) => ({ key: "B" + b.id, text: `${b.kind}(${b.left}${b.right ? ", " + b.right : ""})` })),
  ];

  return (
    <div className="card">
      <h3>Execution trace</h3>
      <table className="table">
        <tbody>
          {rows.map((r) => (
            <tr key={r.key}>
              <td className="mono">{r.key}</td>
              <td className="mono">{r.text}</td>
              <td><span className={results[r.key] ? "badge yes" : "badge no"}>{String(!!results[r.key])}</span></td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}