// Three columns: alpha tests -> beta combiners -> rule terminals.
// `results` (optional) colors nodes green/grey after a simulation run.
const ROW = 54, X = { A: 20, B: 300, T: 580 }, W = 220, H = 36;

export default function NetworkGraph({ network, results, firedNames = [] }) {
  if (!network) return <div className="card">No network yet.</div>;
  const { alphas = [], betas = [], terminals = [] } = network;

  const pos = {};
  alphas.forEach((a, i) => (pos["A" + a.id] = { x: X.A, y: 20 + i * ROW }));
  betas.forEach((b, i) => (pos["B" + b.id] = { x: X.B, y: 20 + i * ROW }));
  terminals.forEach((t, i) => (pos["T" + i] = { x: X.T, y: 20 + i * ROW }));

  const edges = [];
  betas.forEach((b) => {
    edges.push([b.left, "B" + b.id]);
    if (b.right) edges.push([b.right, "B" + b.id]);
  });
  terminals.forEach((t, i) => edges.push([t.root, "T" + i]));

  const rows = Math.max(alphas.length, betas.length, terminals.length);
  const height = 40 + rows * ROW;

  const cls = (key, fired) => {
    if (fired !== undefined) return fired ? "gnode on" : "gnode";
    if (!results) return "gnode";
    return results[key] ? "gnode on" : "gnode off";
  };

  return (
    <div className="card graph">
      <h3>Rete network</h3>
      <svg viewBox={`0 0 840 ${height}`} width="100%">
        {edges.map(([from, to], i) => {
          const a = pos[from], b = pos[to];
          if (!a || !b) return null;
          const live = results && results[from];
          return (
            <line key={i} x1={a.x + W} y1={a.y + H / 2} x2={b.x} y2={b.y + H / 2}
              className={live ? "edge live" : "edge"} />
          );
        })}
        {alphas.map((a) => (
          <g key={"A" + a.id} transform={`translate(${pos["A" + a.id].x},${pos["A" + a.id].y})`}>
            <rect width={W} height={H} rx="6" className={cls("A" + a.id)} />
            <text x="10" y="22">A{a.id}: {a.label.length > 28 ? a.label.slice(0, 27) + "..." : a.label}</text>
          </g>
        ))}
        {betas.map((b) => (
          <g key={"B" + b.id} transform={`translate(${pos["B" + b.id].x},${pos["B" + b.id].y})`}>
            <rect width={W} height={H} rx="18" className={cls("B" + b.id)} />
            <text x="10" y="22">B{b.id}: {b.kind}({b.left}{b.right ? ", " + b.right : ""})</text>
          </g>
        ))}
        {terminals.map((t, i) => (
          <g key={t.ruleName} transform={`translate(${pos["T" + i].x},${pos["T" + i].y})`}>
            <rect width={W} height={H} rx="6"
              className={cls("T" + i, results ? firedNames.includes(t.ruleName) : undefined)} />
            <text x="10" y="22">{t.ruleName} (p{t.priority})</text>
          </g>
        ))}
        <text x={X.A} y={12} className="col">ALPHA (tests)</text>
        <text x={X.B} y={12} className="col">BETA (AND / OR / NOT)</text>
        <text x={X.T} y={12} className="col">TERMINAL (rules)</text>
      </svg>
    </div>
  );
}