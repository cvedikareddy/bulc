import RuleList from "../components/RuleList.jsx";
import NetworkGraph from "../components/NetworkGraph.jsx";
import { Stat } from "../components/CompilationView.jsx";

const STAGES = ["Lexer", "Parser", "Semantic", "IR", "Optimizer", "Network"];

export default function Dashboard({ result, loading, goTo }) {
  const net = result?.network;
  const hasErrors = (result?.diagnostics || []).some((d) => d.severity === "error");
  const stats = result?.stats || {};

  return (
    <div className="stack">
      <div className="card">
        <h3>Compiler pipeline</h3>
        <div className="pipeline">
          {STAGES.map((s, i) => (
            <div key={s} className="stage" onClick={() => goTo("Compiler")}>
              <div className={"dot " + (loading ? "wait" : hasErrors ? "bad" : result ? "good" : "wait")} />
              <span>{s}</span>
              {i < STAGES.length - 1 && <span className="arrow">&rarr;</span>}
            </div>
          ))}
        </div>
      </div>

      <div className="stats">
        <Stat label="Rules" value={result?.rules?.length} />
        <Stat label="Alpha nodes" value={net?.alphas?.length} />
        <Stat label="Beta nodes" value={net?.betas?.length} />
        <Stat label="Terminals" value={net?.terminals?.length} />
        <Stat label="IR nodes" value={stats.nodesBefore != null ? `${stats.nodesBefore} → ${stats.nodesAfter}` : null} />
      </div>

      <div className="cols2">
        <RuleList rules={result?.rules} />
        <NetworkGraph network={net} />
      </div>
    </div>
  );
}