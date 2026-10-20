import { useState } from "react";
import { evaluateFact } from "../services/api.js";
import NetworkGraph from "../components/NetworkGraph.jsx";
import ExecutionTrace from "../components/ExecutionTrace.jsx";
import ResultPanel from "../components/ResultPanel.jsx";

const DEFAULT_FACT = `{
  "amount": 15000,
  "risk_score": 90,
  "country": "US",
  "is_trusted_device": false
}`;

export default function Simulation({ source, result }) {
  const [text, setText] = useState(DEFAULT_FACT);
  const [out, setOut] = useState(null);
  const [error, setError] = useState("");

  const run = async () => {
    setError("");
    try {
      const fact = JSON.parse(text);
      setOut(await evaluateFact(source, result.network, fact));
    } catch (e) {
      setError("Invalid fact JSON: " + e.message);
    }
  };

  return (
    <div className="stack">
      <div className="card">
        <div className="card-head">
          <h3>Fact (incoming event)</h3>
          <button className="btn" onClick={run} disabled={!result?.network}>Run</button>
        </div>
        <textarea className="editor short" value={text} onChange={(e) => setText(e.target.value)} spellCheck={false} />
        {error && <p className="error">{error}</p>}
      </div>

      <div className="cols2">
        <ResultPanel fired={out?.fired} />
        <ExecutionTrace network={result?.network} results={out?.results} />
      </div>

      <NetworkGraph
        network={result?.network}
        results={out?.results}
        firedNames={(out?.fired || []).map((t) => t.ruleName)}
      />
    </div>
  );
}