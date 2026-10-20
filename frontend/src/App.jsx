import { useEffect, useState } from "react";
import { compileRules, SAMPLE_SOURCE } from "./services/api.js";
import Dashboard from "./pages/Dashboard.jsx";
import Rules from "./pages/Rules.jsx";
import Compiler from "./pages/Compiler.jsx";
import Simulation from "./pages/Simulation.jsx";

const PAGES = ["Dashboard", "Rules", "Compiler", "Simulation"];

export default function App() {
  const [page, setPage] = useState("Dashboard");
  const [source, setSource] = useState(SAMPLE_SOURCE);
  const [result, setResult] = useState(null);
  const [loading, setLoading] = useState(false);

  const compile = async () => {
    setLoading(true);
    try {
      setResult(await compileRules(source));
    } catch (e) {
      setResult({ diagnostics: [{ severity: "error", message: String(e) }] });
    }
    setLoading(false);
  };

  useEffect(() => { compile(); }, []); // compile the sample once on load

  const shared = { source, setSource, result, loading, compile };

  return (
    <div className="app">
      <header className="topbar">
        <h1>BULC <span>Rule Engine Compiler</span></h1>
        <nav>
          {PAGES.map((p) => (
            <button key={p} className={p === page ? "tab active" : "tab"} onClick={() => setPage(p)}>
              {p}
            </button>
          ))}
        </nav>
      </header>
      <main>
        {page === "Dashboard" && <Dashboard {...shared} goTo={setPage} />}
        {page === "Rules" && <Rules {...shared} />}
        {page === "Compiler" && <Compiler {...shared} />}
        {page === "Simulation" && <Simulation {...shared} />}
      </main>
    </div>
  );
}